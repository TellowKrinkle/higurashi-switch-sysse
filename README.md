# Higurashi Switch sysse.bin tools
To extract files from `sysse.bin` archive:
- `gcc sysse-extract.c -o sysse-extract`
- `./sysse-extract sysse.bin output-directory`

To convert extracted files to wav files:
- `gcc sysse-dec.c -o sysse-dec`
- `./sysse-dec file.bin file.wav`
