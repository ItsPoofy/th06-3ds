"""Readable conversion errors and persistent diagnostic reports."""
from datetime import datetime
from pathlib import Path
import tempfile
import traceback


def write_error_log(error, stage, source, output, directory):
    report = (
        f'TH06 Asset Converter — {datetime.now().astimezone().isoformat(timespec="seconds")}\n'
        f'Stage: {stage}\nSource: {source}\nOutput: {output}\n\n'
        + ''.join(traceback.format_exception(error))
    )
    path = Path(directory)/'TH06-Asset-Converter-error.log'
    try:
        path.write_text(report, encoding='utf-8')
        return path
    except OSError:
        try:
            with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8',
                    prefix='TH06-Asset-Converter-error-', suffix='.log', delete=False) as file:
                file.write(report)
                return Path(file.name)
        except OSError:
            return None


def error_message(error, stage, log_path):
    reason = str(error).strip() or 'No additional error details were provided.'
    message = f'{type(error).__name__}: {reason}\n\nWhile: {stage}'
    if log_path:
        message += f'\n\nDiagnostic log:\n{log_path}'
    else:
        message += '\n\nThe diagnostic log could not be saved.'
    return message
