/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * hessian_in.c
 *
 *  \date       Aug 1, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>

#include "hessian_2.0.h"

static int END_OF_DATA = -2;

int hessian_parseChar(hessian_in_t in, char *c);
int hessian_parseLong(hessian_in_t in, long *value);
int hessian_parseInt16(hessian_in_t in, int *value);
int hessian_parseInt(hessian_in_t in, int *value);

char hessian_read(hessian_in_t in) {
	return (in->buffer[in->offset++] & 0xFF);
}

int hessian_readBoolean(hessian_in_t in, bool *value) {
	char tag = hessian_read(in);

	switch (tag) {
		case 'T':
			*value = true;
			break;
		case 'F':
		default:
			*value = false;
			break;
	}

	return 0;
}

int hessian_readInt(hessian_in_t in, int *value) {
	unsigned char tag = hessian_read(in);

	switch (tag) {
		// direct integer
		case 0x80: case 0x81: case 0x82: case 0x83:
		case 0x84: case 0x85: case 0x86: case 0x87:
		case 0x88: case 0x89: case 0x8a: case 0x8b:
		case 0x8c: case 0x8d: case 0x8e: case 0x8f:

		case 0x90: case 0x91: case 0x92: case 0x93:
		case 0x94: case 0x95: case 0x96: case 0x97:
		case 0x98: case 0x99: case 0x9a: case 0x9b:
		case 0x9c: case 0x9d: case 0x9e: case 0x9f:

		case 0xa0: case 0xa1: case 0xa2: case 0xa3:
		case 0xa4: case 0xa5: case 0xa6: case 0xa7:
		case 0xa8: case 0xa9: case 0xaa: case 0xab:
		case 0xac: case 0xad: case 0xae: case 0xaf:

		case 0xb0: case 0xb1: case 0xb2: case 0xb3:
		case 0xb4: case 0xb5: case 0xb6: case 0xb7:
		case 0xb8: case 0xb9: case 0xba: case 0xbb:
		case 0xbc: case 0xbd: case 0xbe: case 0xbf:
			*value = tag - INT_ZERO;
			break;

		/* byte int */
		case 0xc0: case 0xc1: case 0xc2: case 0xc3:
		case 0xc4: case 0xc5: case 0xc6: case 0xc7:
		case 0xc8: case 0xc9: case 0xca: case 0xcb:
		case 0xcc: case 0xcd: case 0xce: case 0xcf:
			*value = ((tag - INT_BYTE_ZERO) << 8) + hessian_read(in);
			break;

		/* short int */
		case 0xd0: case 0xd1: case 0xd2: case 0xd3:
		case 0xd4: case 0xd5: case 0xd6: case 0xd7:
			*value = ((tag - INT_SHORT_ZERO) << 16) + 256 * hessian_read(in) + hessian_read(in);
			break;

		case 'I':
			hessian_readInt(in, value);
			break;

		default:
			*value = 0;
			break;
	}

	return 1;
}

int hessian_readLong(hessian_in_t in, long *value) {
	unsigned char tag = hessian_read(in);

	switch (tag) {
		// direct long
		case 0xd8: case 0xd9: case 0xda: case 0xdb:
		case 0xdc: case 0xdd: case 0xde: case 0xdf:

		case 0xe0: case 0xe1: case 0xe2: case 0xe3:
		case 0xe4: case 0xe5: case 0xe6: case 0xe7:
		case 0xe8: case 0xe9: case 0xea: case 0xeb:
		case 0xec: case 0xed: case 0xee: case 0xef:
			*value = tag - LONG_ZERO;
			break;

		/* byte long */
		case 0xf0: case 0xf1: case 0xf2: case 0xf3:
		case 0xf4: case 0xf5: case 0xf6: case 0xf7:
		case 0xf8: case 0xf9: case 0xfa: case 0xfb:
		case 0xfc: case 0xfd: case 0xfe: case 0xff:
			*value = ((tag - LONG_BYTE_ZERO) << 8) + hessian_read(in);
			break;

		/* short long */
		case 0x38: case 0x39: case 0x3a: case 0x3b:
		case 0x3c: case 0x3d: case 0x3e: case 0x3f:
			*value = ((tag - LONG_SHORT_ZERO) << 16) + 256
					* hessian_read(in) + hessian_read(in);
			break;

		case 'L':
			hessian_parseLong(in, value);
			break;
		default:
			return 0l;
	}

	return 0;
}

int hessian_readDouble(hessian_in_t in, double *value) {
	unsigned char tag = hessian_read(in);

	long l;
	int i;
	double *d;
	switch (tag) {
		case 0x67:
			*value = 0;
			break;
	    case 0x68:
	    	*value = 1;
	    	break;
	    case 0x69:
	    	*value = (double) hessian_read(in);
	    	break;
	    case 0x6a:
	    	*value = (short) (256 * hessian_read(in) + hessian_read(in));
	    	break;
	    case 0x6b:
	    	hessian_parseInt(in, &i);

	    	*value = (double) *((float *) &i);
	    	break;
	    case 'D':
	    	hessian_parseLong(in, &l);

	    	d = (double *) &l;

	    	*value = *d;
	    	break;
	}

	return 0;
}

int hessian_readUTCDate(hessian_in_t in, long *value) {
	unsigned char tag = hessian_read(in);

	if (tag == 'd') {
		hessian_parseLong(in, value);
	}

	return 0;
}

int hessian_readNull(hessian_in_t in) {
	unsigned char tag = hessian_read(in);

	switch (tag) {
		case 'N':
			break;
		default:
			break;
	}

	return 0;
}

int hessian_readChar(hessian_in_t in, char *value) {
	char *readC;
	unsigned int read;
	hessian_readNString(in, 0, 1, &readC, &read);
	*value = readC[0];

	return 0;
}

int hessian_readString(hessian_in_t in, char **value, unsigned int *readLength) {
	return hessian_readNString(in, 0, -1, value, readLength);
}

int hessian_readNString(hessian_in_t in, int offset, int length, char **value, unsigned int *readLength) {
	*readLength = 0;

	bool done = false;
	if (in->chunkLength == END_OF_DATA) {
		done = true;
	} else if (in->chunkLength == 0) {
		unsigned char tag = hessian_read(in);

		switch (tag) {
			case 'N':
				in->chunkLength = 0;
				in->lastChunk = true;
				break;
			case 'S':
			case 's':
				in->lastChunk = tag == 'S';
				hessian_parseInt16(in, &(in->chunkLength));
				break;

			case 0x00: case 0x01: case 0x02: case 0x03:
			case 0x04: case 0x05: case 0x06: case 0x07:
			case 0x08: case 0x09: case 0x0a: case 0x0b:
			case 0x0c: case 0x0d: case 0x0e: case 0x0f:

			case 0x10: case 0x11: case 0x12: case 0x13:
			case 0x14: case 0x15: case 0x16: case 0x17:
			case 0x18: case 0x19: case 0x1a: case 0x1b:
			case 0x1c: case 0x1d: case 0x1e: case 0x1f:
				in->chunkLength = tag - 0x00;
				in->lastChunk = true;
				break;

			case 0x30: case 0x31: case 0x32: case 0x33:
				in->chunkLength = (tag - 0x30) * 256 + hessian_read(in);
				in->lastChunk = true;
				break;

			default:
				in->chunkLength = 0;
				in->lastChunk = true;
				break;
		}
	}

	while (!done) {
		unsigned int newSize = *readLength + in->chunkLength + 1;
		*value = realloc(*value, sizeof(char) * (*readLength + in->chunkLength + 1));

		if (in->chunkLength > 0) {
			char c;
			hessian_parseChar(in, &c);
			(*value)[(*readLength) + offset] = c;
			(*value)[(*readLength) + offset + 1] = '\0';
			in->chunkLength--;
			(*readLength)++;
			length--;

			if (length == 0) {
				done = true;
			}

		} else if (in->lastChunk) {
			done = true;
			if (*readLength != 0) {
				in->chunkLength = END_OF_DATA;
			}
		} else {
			unsigned char tag = hessian_read(in);
			switch (tag) {
				case 'S':
				case 's':
					done = tag == 'S';

					hessian_parseInt16(in, &in->chunkLength);
					break;

				case 0x00: case 0x01: case 0x02: case 0x03:
				case 0x04: case 0x05: case 0x06: case 0x07:
				case 0x08: case 0x09: case 0x0a: case 0x0b:
				case 0x0c: case 0x0d: case 0x0e: case 0x0f:

				case 0x10: case 0x11: case 0x12: case 0x13:
				case 0x14: case 0x15: case 0x16: case 0x17:
				case 0x18: case 0x19: case 0x1a: case 0x1b:
				case 0x1c: case 0x1d: case 0x1e: case 0x1f:
					in->chunkLength = tag - 0x00;
					in->lastChunk = true;
					break;

				case 0x30: case 0x31: case 0x32: case 0x33:
					in->chunkLength = (tag - 0x30) * 256 + hessian_read(in);
					in->lastChunk = true;
					break;

				default:
					in->chunkLength = 0;
					in->lastChunk = true;
				break;
			}
		}
	}

	if (in->lastChunk && in->chunkLength == 0) {
		in->chunkLength = END_OF_DATA;
	}

	return 0;
}

int hessian_readByte(hessian_in_t in, unsigned char *value) {
	unsigned char *readC;
	unsigned int read;
	hessian_readNBytes(in, 0, 1, &readC, &read);
	*value = readC[0];

	return 0;
}

int hessian_readBytes(hessian_in_t in, unsigned char **value, unsigned int *readLength) {
	return hessian_readNBytes(in, 0, -1, value, readLength);
}

int hessian_readNBytes(hessian_in_t in, int offset, int length, unsigned char **value, unsigned int *readLength) {
	*readLength = 0;

	bool done = false;
	if (in->chunkLength == END_OF_DATA) {
		done = true;
	} else if (in->chunkLength == 0) {
		unsigned char tag = hessian_read(in);

		switch (tag) {
			case 'N':
				in->chunkLength = 0;
				in->lastChunk = true;
				break;
			case 'B':
			case 'b':
				in->lastChunk = tag == 'B';
				hessian_parseInt16(in, &(in->chunkLength));
				break;

			case 0x20: case 0x21: case 0x22: case 0x23:
			case 0x24: case 0x25: case 0x26: case 0x27:
			case 0x28: case 0x29: case 0x2a: case 0x2b:
			case 0x2c: case 0x2d: case 0x2e: case 0x2f:
				in->chunkLength = tag - 0x20;
				in->lastChunk = true;
				break;

			default:
				in->chunkLength = 0;
				in->lastChunk = true;
				break;
		}
	}

	while (!done) {
		unsigned int newSize = *readLength + in->chunkLength;
		*value = realloc(*value, sizeof(char) * (*readLength + in->chunkLength));

		if (in->chunkLength > 0) {
			unsigned char c = hessian_read(in);
			(*value)[(*readLength) + offset] = c;
			in->chunkLength--;
			(*readLength)++;
			length--;

			if (length == 0) {
				done = true;
			}

		} else if (in->lastChunk) {
			done = true;
			if (*readLength != 0) {
				in->chunkLength = END_OF_DATA;
			}
		} else {
			unsigned char tag = hessian_read(in);
			switch (tag) {
				case 'B':
				case 'b':
					done = tag == 'S';

					hessian_parseInt16(in, &in->chunkLength);
					break;

				case 0x20: case 0x21: case 0x22: case 0x23:
				case 0x24: case 0x25: case 0x26: case 0x27:
				case 0x28: case 0x29: case 0x2a: case 0x2b:
				case 0x2c: case 0x2d: case 0x2e: case 0x2f:
					in->chunkLength = tag - 0x20;
					in->lastChunk = true;
					break;

				default:
					in->chunkLength = 0;
					in->lastChunk = true;
				break;
			}
		}
	}

	if (in->lastChunk && in->chunkLength == 0) {
		in->chunkLength = END_OF_DATA;
	}

	return 0;
}

int hessian_parseChar(hessian_in_t in, char *c) {
	unsigned char ch = hessian_read(in);

	if (ch < 0x80) {
		*c = ch;
	} else if ((ch & 0xe0) == 0xc0) {
		char ch1 = hessian_read(in);

		*c = ((ch & 0x1f) << 6) + (ch1 & 0x3f);
	} else if ((ch & 0xf0) == 0xe0) {
		char ch1 = hessian_read(in);
		char ch2 = hessian_read(in);

		*c = ((ch & 0x0f) << 12) + ((ch1 & 0x3f) << 6) + (ch2 & 0x3f);
	} else {
		*c = '\0';
	}

	return 0;
}

int hessian_parseInt16(hessian_in_t in, int *value) {
	int b16 = hessian_read(in) & 0xFF;
	int b8 = hessian_read(in) & 0xFF;

	*value = (b16 << 8)
			+ b8;

	return 0;
}

int hessian_parseInt(hessian_in_t in, int *value) {
	int b32 = hessian_read(in) & 0xFF;
	int b24 = hessian_read(in) & 0xFF;
	int b16 = hessian_read(in) & 0xFF;
	int b8 = hessian_read(in) & 0xFF;

	*value = (b32 << 24)
			+ (b24 << 16)
			+ (b16 << 8)
			+ b8;

	return 0;
}

int hessian_parseLong(hessian_in_t in, long *value) {
	long b64 = hessian_read(in) & 0xFF;
	long b56 = hessian_read(in) & 0xFF;
	long b48 = hessian_read(in) & 0xFF;
	long b40 = hessian_read(in) & 0xFF;
	long b32 = hessian_read(in) & 0xFF;
	long b24 = hessian_read(in) & 0xFF;
	long b16 = hessian_read(in) & 0xFF;
	long b8 = hessian_read(in) & 0xFF;

	long v = (b64 << 56)
			+ (b56 << 48)
			+ (b48 << 40)
			+ (b40 << 32)
			+ (b32 << 24)
			+ (b24 << 16)
			+ (b16 << 8)
			+ b8;

	*value = v;

	return 0;
}
