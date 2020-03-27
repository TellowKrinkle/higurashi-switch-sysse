#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

struct Header {
	uint32_t magic;
	uint32_t filesize;
	uint32_t numEntries;
};

static const int ENTRY_NAME_LEN = 16;

struct Entry {
	char name[ENTRY_NAME_LEN];
	uint32_t offset;
	uint32_t size;
};

int main(int argc, char const *argv[]) {
	if (argc <= 2) {
		fprintf(stderr, "Usage: %s sysse.bin outfolder\n", argv[0]);
		return EXIT_FAILURE;
	}

	int ret = EXIT_FAILURE;

	char const *inFileName = argv[1];
	char const *outFolderName = argv[2];
	char outFileName[4096];
	strncpy(outFileName, outFolderName, sizeof(outFileName) - (ENTRY_NAME_LEN + 1));
	char *outFileWrite = outFileName + strlen(outFileName);
	*outFileWrite++ = '/';

	FILE *inFile = fopen(inFileName, "r");
	if (!inFile) { 
		fprintf(stderr, "Failed to open %s for reading\n", inFileName);
		goto failedToOpenInput;
	}
	struct Header header;
	if (1 != fread(&header, sizeof(header), 1, inFile)) {
		fprintf(stderr, "Failed to read header from %s\n", inFileName);
		goto failedWithInput;
	}
	struct Entry *entries = calloc(header.numEntries, sizeof(struct Entry));
	void *buffer = NULL;
	if (header.numEntries != fread(entries, sizeof(struct Entry), header.numEntries, inFile)) {
		fprintf(stderr, "Failed to read file entries from %s\n", inFileName);
		goto failedWithEntries;
	}

	ret = EXIT_SUCCESS;
	for (int i = 0; i < header.numEntries; i++) {
		struct Entry *entry = &entries[i];
		buffer = realloc(buffer, entry->size);
		strncpy(outFileWrite, entry->name, (ENTRY_NAME_LEN + 1));
		printf("%s: offset %d size %d\n", outFileWrite, entry->offset, entry->size);
		fseek(inFile, entry->offset, SEEK_SET);
		if (1 != fread(buffer, entry->size, 1, inFile)) {
			fprintf(stderr, "Failed to read %s from %s\n", outFileWrite, inFileName);
			ret = EXIT_FAILURE;
			continue;
		}
		FILE *outFile = fopen(outFileName, "w");
		if (!outFile) {
			fprintf(stderr, "Failed to open %s for writing\n", outFileName);
			ret = EXIT_FAILURE;
			continue;
		}
		if (1 != fwrite(buffer, entry->size, 1, outFile)) {
			fprintf(stderr, "Failed to write to %s\n", outFileName);
			ret = EXIT_FAILURE;
		}
		fclose(outFile);
	}

failedWithEntries:
	if (buffer) { free(buffer); }
	free(entries);
failedWithInput:
	fclose(inFile);
failedToOpenInput:
	return ret;
}
