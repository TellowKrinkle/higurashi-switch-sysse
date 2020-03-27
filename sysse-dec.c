#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

const int decArray[10] = {
	  0,   0,
	 60,   0,
	115, -52,
	 98, -55,
	122, -60,
};

void decode(uint8_t *in, int16_t *out, size_t length, size_t channels) {
	uint8_t *pos = in;
	int16_t prev[channels], prevprev[channels];
	memset(prev, 0, sizeof(prev));
	memset(prevprev, 0, sizeof(prevprev));
	for (int j = 0; j < length/16; j++) {
		const int channel = j % channels;
		int16_t *outpos = out + (30 * (j / channels * channels) + channel);
		uint8_t read = *pos++;
		const int shift = read & 0xF;
		const int index = ((read & 0xF0) >> 1) / sizeof(*decArray);
		const int multiple1 = decArray[index];
		const int multiple2 = decArray[index + 1];
		for (int i = 0; i < 30; i++) {
			const uint8_t nybble = ((i & 1) == 0) ? (read = *pos++) & 0xF : read >> 4;
			const int snybble = nybble < 8 ? nybble : nybble | 0xFFFFFFF0;
			const int base = prev[channel] * multiple1 + prevprev[channel] * multiple2;
			const int offsetBase = base + 0x20 >= 0 ? base + 0x20 : base + 0x5f;
			const int shifted = (snybble << shift) + (offsetBase >> 6);
			const int16_t clamped = shifted > 32766 ? 32767 : shifted < -32767 ? -32768 : shifted;
			*outpos = clamped;
			outpos += channels;
			prevprev[channel] = prev[channel];
			prev[channel] = clamped;
		}
	}
}

struct Header {
	char magic[4];
	int size;
	short channels;
	short samplerate;
	int unk;
};

bool writeWav(FILE *out, int16_t *data, int channels, int samplerate, int samples);

int main(int argc, const char * argv[]) {
	if (argc <= 2) {
		fprintf(stderr, "Usage: %s in.bin out.wav\n", argv[0]);
		return EXIT_FAILURE;
	}
	FILE *in = fopen(argv[1], "r");
	if (!in) {
		fprintf(stderr, "Failed to open %s for reading\n", argv[1]);
		return EXIT_FAILURE;
	}
	struct Header header;
	if (fread(&header, sizeof(header), 1, in) != 1) {
		fprintf(stderr, "Failed to read header\n");
		return EXIT_FAILURE;
	}
	if (memcmp(header.magic, "ADP1", 4) != 0) {
		fprintf(stderr, "Bad magic %c%c%c%c!\n", header.magic[0], header.magic[1], header.magic[2], header.magic[3]);
		return EXIT_FAILURE;
	}
	const int size = header.size - sizeof(header);
	const int outSize = size / 16 * 30 * 2;
	void *inBuf = malloc(size);
	void *outBuf = malloc(size / 16 * 30 * 2);
	if (fread(inBuf, 1, size, in) != size) {
		fprintf(stderr, "Failed to read audio data\n");
		return EXIT_FAILURE;
	}
	fclose(in);
	decode(inBuf, outBuf, size, header.channels);
	FILE *out = fopen(argv[2], "w");
	if (!out) {
		fprintf(stderr, "Failed to open %s for writing\n", argv[2]);
		return EXIT_FAILURE;
	}
	if (!writeWav(out, outBuf, header.channels, header.samplerate, outSize/sizeof(int16_t))) {
		fprintf(stderr, "Failed to write audio data\n");
		return EXIT_FAILURE;
	}
	fclose(out);
	return 0;
}

const char chunkID[4] = "RIFF";
const char format[4] = "WAVE";
const char fmtChunkID[4] = "fmt ";
const char dataChunkID[4] = "data";

struct WavHeader {
	char chunkID[4];
	uint32_t chunkSize;
	char format[4];
};

struct WavFormat {
	char subchunkID[4];
	uint32_t subchunkSize;
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
};

struct WavData {
	char subchunkID[4];
	uint32_t subchunkSize;
};

bool writeWav(FILE *out, int16_t *data, int channels, int samplerate, int samples) {
	struct WavHeader header = {
		"RIFF",
		samples * 2 + 36,
		"WAVE"
	};

	struct WavFormat formatHeader = {
		.subchunkID = "fmt ",
		.subchunkSize = 16,
		.audioFormat = 1,
		.numChannels = channels,
		.sampleRate = samplerate,
		.byteRate = channels * samplerate * sizeof(int16_t),
		.blockAlign = channels * sizeof(int16_t),
		.bitsPerSample = sizeof(int16_t) * 8
	};

	struct WavData dataHeader = {
		"data",
		samplerate * sizeof(int16_t)
	};

	if (fwrite(&header, sizeof(header), 1, out) != 1) { return false; }
	if (fwrite(&formatHeader, sizeof(formatHeader), 1, out) != 1) { return false; }
	if (fwrite(&dataHeader, sizeof(dataHeader), 1, out) != 1) { return false; }
	if (fwrite(data, sizeof(int16_t), samples, out) != samples) { return false; }
	return true;
}
