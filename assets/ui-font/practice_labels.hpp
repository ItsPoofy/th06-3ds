#pragma once
namespace CtrPracticeAssets {
struct Label {const char* text;int x,y,w,h;float faceTop,faceBottom;};
inline constexpr Label labels[]={
{"STAGE 1",1,1,100,28,6,20},
{"STAGE 2",102,1,100,28,6,20},
{"STAGE 3",203,1,100,28,6,20},
{"STAGE 4",304,1,100,28,6,20},
{"STAGE 5",405,1,100,28,6,20},
{"STAGE 6",1,30,100,28,6,20},
{"character-prompt-native-en",102,30,290,24,0,23},
{"weapon-prompt-native-en",1,59,290,24,0,23},
{"difficulty-prompt-native-en",1,84,290,24,0,23},
{"character-prompt-native",1,109,246,24,0,23},
{"weapon-prompt-native",1,134,265,23,0,22},
{"difficulty-prompt-native",292,59,167,23,0,22},
};

}
