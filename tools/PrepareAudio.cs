using System;
using System.IO;
using System.Collections.Generic;
using VGAudio.Codecs.GcAdpcm;
class PrepareAudio {
 class Block {public uint samples,bytes,offset;public short[][] history;public byte[][] audio;}
 static void Main(string[] a) {
  foreach(string path in Directory.GetFiles(a[0],"*.wav")) {
   string name=Path.GetFileNameWithoutExtension(path),pos=Path.Combine(a[1],name+".pos");
   if(!File.Exists(pos))continue;
   int rate=0,channels=0,bits=0;byte[] raw=null;
   using(var r=new BinaryReader(File.OpenRead(path))) {
    if(new string(r.ReadChars(4))!="RIFF")throw new Exception(path);r.ReadUInt32();r.ReadChars(4);
    while(r.BaseStream.Position+8<=r.BaseStream.Length){string id=new string(r.ReadChars(4));uint n=r.ReadUInt32();long end=r.BaseStream.Position+n+(n&1);
     if(id=="fmt "){if(r.ReadUInt16()!=1)throw new Exception("PCM WAV required");channels=r.ReadUInt16();rate=r.ReadInt32();r.ReadUInt32();r.ReadUInt16();bits=r.ReadUInt16();}
     if(id=="data")raw=r.ReadBytes((int)n);r.BaseStream.Position=end;
    }
   }
   if(channels!=2||bits!=16||raw==null)throw new Exception("Expected original stereo PCM16 WAV");
   uint loopStart,loopEnd;using(var r=new BinaryReader(File.OpenRead(pos))){loopStart=r.ReadUInt32();loopEnd=r.ReadUInt32();}
   if(loopStart>=loopEnd||loopEnd>raw.Length/4)throw new Exception("Bad original loop points");
   short[][] pcm=new short[2][],coefs=new short[2][];short[] h1=new short[2],h2=new short[2];
   for(int c=0;c<2;c++){pcm[c]=new short[loopEnd];for(int i=0;i<loopEnd;i++)pcm[c][i]=BitConverter.ToInt16(raw,i*4+c*2);coefs[c]=GcAdpcmCoefficients.CalculateCoefficients(pcm[c]);}
   var blocks=new List<Block>();int loopBlock=0;
   for(int segment=0;segment<2;segment++) {
    uint start=segment==0?0:loopStart,end=segment==0?loopStart:loopEnd;
    if(segment==1)loopBlock=blocks.Count;
    for(uint p=start;p<end;) {
     int n=(int)Math.Min(14336,end-p);var b=new Block{samples=(uint)n,bytes=(uint)((n+13)/14*8),history=new short[2][],audio=new byte[2][]};
     for(int c=0;c<2;c++) {
      b.history[c]=new short[]{h1[c],h2[c]};var samples=new short[n];Array.Copy(pcm[c],p,samples,0,n);
      byte[] encoded=GcAdpcmEncoder.Encode(samples,coefs[c],new GcAdpcmParameters{History1=h1[c],History2=h2[c]});
      b.audio[c]=new byte[b.bytes];Array.Copy(encoded,b.audio[c],encoded.Length);
      for(int i=0;i<n;i++) {int frame=i/14*8,k=i%14,header=b.audio[c][frame],v=b.audio[c][frame+1+k/2];v=k%2==0?v>>4:v&15;if(v>=8)v-=16;
       int value=((v*(1<<(header&15)))<<11)+1024+coefs[c][(header>>4)*2]*h1[c]+coefs[c][(header>>4)*2+1]*h2[c];value=Math.Max(-32768,Math.Min(32767,value>>11));h2[c]=h1[c];h1[c]=(short)value;
      }
     }
     blocks.Add(b);p+=(uint)n;
    }
   }
   Directory.CreateDirectory(a[2]);using(var w=new BinaryWriter(File.Create(Path.Combine(a[2],name+".ndsp")))) {
    w.Write(new byte[]{78,68,83,80});w.Write(1u);w.Write((uint)rate);w.Write(2u);w.Write((uint)blocks.Count);w.Write((uint)loopBlock);w.Write(loopEnd);
    for(int c=0;c<2;c++)foreach(short coef in coefs[c])w.Write(coef);
    uint offset=(uint)(92+24*blocks.Count);
    foreach(var b in blocks){b.offset=offset;w.Write(b.samples);w.Write(b.bytes);w.Write(offset);for(int c=0;c<2;c++){w.Write((ushort)b.audio[c][0]);w.Write(b.history[c][0]);w.Write(b.history[c][1]);}offset+=b.bytes*2;}
    foreach(var b in blocks)for(int c=0;c<2;c++)w.Write(b.audio[c]);
   }
   Console.WriteLine(name+": ADPCM, original "+rate+" Hz stereo, exact loop "+loopStart+".."+loopEnd);
  }
 }
}
