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
 * hessian.c
 *
 *  \date       Jul 31, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>

#include "hessian_2.0.h"

void hessian_ensureCapacity(hessian_out_t obj, int capacity);

int hessian_printString(hessian_out_t out, char *value);
int hessian_printNString(hessian_out_t out, char *value, int offset, int length);

int hessian_writeType(hessian_out_t out, char *type);

int hessian_write(hessian_out_t out, unsigned char byte);
int hessian_writeP(hessian_out_t out, unsigned char *byte);

int hessian_writeBoolean(hessian_out_t out, bool value) {
	hessian_ensureCapacity(out, out->offset + 1);

	if (value) {
		char c = 'T';
		hessian_writeP(out, &c);
	} else {
		out->buffer[out->offset++] = 'F';
		out->length++;
	}

	return 0;
}

int hessian_writeInt(hessian_out_t out, int value) {
	hessian_ensureCapacity(out, out->offset + 5);

	if (INT_DIRECT_MIN <= value && value <= INT_DIRECT_MAX)
		hessian_write(out, (char) (value + INT_ZERO));
	else if (INT_BYTE_MIN <= value && value <= INT_BYTE_MAX) {
		hessian_write(out, (char) (INT_BYTE_ZERO + (value >> 8)));
		hessian_write(out, (char) (value));
	} else if (INT_SHORT_MIN <= value && value <= INT_SHORT_MAX) {
		hessian_write(out, (char)(INT_SHORT_ZERO + (value >> 16)));
		hessian_write(out, (char)(value >> 8));
		hessian_write(out, (char)(value));
	} else {
		hessian_write(out, (char)('I'));
		hessian_write(out, (char)(value >> 24));
		hessian_write(out, (char)(value >> 16));
		hessian_write(out, (char)(value >> 8));
		hessian_write(out, (char)(value));
	}

	return 0;
}

int hessian_writeLong(hessian_out_t out, long value) {
	hessian_ensureCapacity(out, out->offset + 9);

	if (LONG_DIRECT_MIN <= value && value <= LONG_DIRECT_MAX) {
		hessian_write(out, (char)(value + LONG_ZERO));
	} else if (LONG_BYTE_MIN <= value && value <= LONG_BYTE_MAX) {
		hessian_write(out, (char)(LONG_BYTE_ZERO + (value >> 8)));
		hessian_write(out, (char)(value));
	} else if (LONG_SHORT_MIN <= value && value <= LONG_SHORT_MAX) {
		hessian_write(out, (char)(LONG_SHORT_ZERO + (value >> 16)));
		hessian_write(out, (char)(value >> 8));
		hessian_write(out, (char)(value));
	} else if (-0x80000000L <= value && value <= 0x7fffffffL) {
		hessian_write(out, (char) LONG_INT);
		hessian_write(out, (char)(value >> 24));
		hessian_write(out, (char)(value >> 16));
		hessian_write(out, (char)(value >> 8));
		hessian_write(out, (char)(value));
	} else {
		hessian_write(out, (char) 'L');
		hessian_write(out, (char)(value >> 56));
		hessian_write(out, (char)(value >> 48));
		hessian_write(out, (char)(value >> 40));
		hessian_write(out, (char)(value >> 32));
		hessian_write(out, (char)(value >> 24));
		hessian_write(out, (char)(value >> 16));
		hessian_write(out, (char)(value >> 8));
		hessian_write(out, (char)(value));
	}

	return 0;
}

int hessian_writeDouble(hessian_out_t out, double value) {
	hessian_ensureCapacity(out, out->offset + 9);

	int intValue = (int) value;

	if (intValue == value) {
		if (intValue == 0) {
			hessian_write(out, (char) DOUBLE_ZERO);
		} else if (intValue == 1) {
			hessian_write(out, (char) DOUBLE_ONE);
		} else if (-0x80 <= intValue && intValue < 0x80) {
			hessian_write(out, (char) DOUBLE_BYTE);
			hessian_write(out, (char) intValue);
		} else if (-0x8000 <= intValue && intValue < 0x8000) {
			hessian_write(out, (char) DOUBLE_SHORT);
			hessian_write(out, (char) ((intValue >> 8) & 0xFF));
			hessian_write(out, (char) (intValue & 0xFF));
		}
	} else {
		float f = (float) value;

		if (f == value) {
			float f = value;
			int bits = *((int *) &f);

			hessian_write(out, (char)(DOUBLE_FLOAT));
			hessian_write(out, (char)(bits >> 24));
			hessian_write(out, (char)(bits >> 16));
			hessian_write(out, (char)(bits >> 8));
			hessian_write(out, (char)(bits));
		} else {
			long bits = *((long *) &value);

			hessian_write(out, (char) 'D');
			hessian_write(out, (char)(bits >> 56) & 0x00000000000000FF);
			hessian_write(out, (char)(bits >> 48) & 0x00000000000000FF);
			hessian_write(out, (char)(bits >> 40) & 0x00000000000000FF);
			hessian_write(out, (char)(bits >> 32) & 0x00000000000000FF);
			hessian_write(out, (char)(bits >> 24) & 0x00000000000000FF);
			hessian_write(out, (char)(bits >> 16) & 0x00000000000000FF);
			hessian_write(out, (char)(bits >> 8) & 0x00000000000000FF);
			hessian_write(out, (char)(bits) & 0x00000000000000FF);
		}
	}

	return 0;
}

int hessian_writeUTCDate(hessian_out_t out, long value) {
	hessian_ensureCapacity(out, out->offset + 9);

	hessian_write(out, (int) ('d'));
	hessian_write(out, ((int) (value >> 56)));
	hessian_write(out, ((int) (value >> 48)));
	hessian_write(out, ((int) (value >> 40)));
	hessian_write(out, ((int) (value >> 32)));
	hessian_write(out, ((int) (value >> 24)));
	hessian_write(out, ((int) (value >> 16)));
	hessian_write(out, ((int) (value >> 8)));
	hessian_write(out, ((int) (value)));

	return 0;
}

int hessian_writeNull(hessian_out_t out) {
	hessian_ensureCapacity(out, out->offset + 1);

	hessian_write(out, (int) ('N'));

	return 0;
}

int hessian_writeString(hessian_out_t out, char *value) {
	int length = strlen(value);
	return hessian_writeNString(out, value, 0, length);
}

int hessian_writeNString(hessian_out_t out, char *value, int offset, int length) {
	if (value == NULL) {
		hessian_writeNull(out);
	} else {
		while (length > 0x8000) {
			int sublen = 0x8000;

			// chunk can't end in high surrogate
			int tail = value[offset + sublen - 1];

			if (0xd800 <= tail && tail <= 0xdbff)
				sublen--;

			hessian_ensureCapacity(out, out->offset + 3);

			hessian_write(out, (int) 's');
			hessian_write(out, (int)(sublen >> 8));
			hessian_write(out, (int)(sublen));

			hessian_printNString(out, value, offset, sublen);

			length -= sublen;
			offset += sublen;
		}

		if (length <= STRING_DIRECT_MAX) {
			hessian_ensureCapacity(out, out->offset + 2);

			hessian_write(out, (int)(STRING_DIRECT + length));
		} else {
			hessian_ensureCapacity(out, out->offset + 3);

			hessian_write(out, (int)('S'));
			hessian_write(out, (int)(length >> 8));
			hessian_write(out, (int)(length));
		}

		hessian_printNString(out, value, offset, length);
	}

	return 0;
}

int hessian_writeBytes(hessian_out_t out, unsigned char value[], int length) {
	return hessian_writeNBytes(out, value, 0, length);
}

int hessian_writeNBytes(hessian_out_t out, unsigned char value[], int offset, int length) {
	if (value == NULL) {
		hessian_writeNull(out);
	} else {
		while (length > 0x8000) {
			int sublen = 0x8000;

			hessian_ensureCapacity(out, out->offset + 3);

			hessian_write(out, (int) 'b');
			hessian_write(out, (int)(sublen >> 8));
			hessian_write(out, (int) sublen);

			hessian_ensureCapacity(out, out->offset + sublen);
			memcpy(out->buffer+out->offset, value+offset, sublen);
			out->offset += sublen;

			length -= sublen;
			offset += sublen;
		}

		if (length < 0x10) {
			hessian_ensureCapacity(out, out->offset + 1);
			hessian_write(out, (int)(BYTES_DIRECT + length));
		} else {
			hessian_ensureCapacity(out, out->offset + 3);
			hessian_write(out, (int) 'B');
			hessian_write(out, (int)(length >> 8));
			hessian_write(out, (int)(length));
		}

		hessian_ensureCapacity(out, out->offset + length);
		memcpy(out->buffer+out->offset, value+offset, length);

		out->offset += length;
	}

	return 0;
}

int hessian_writeListBegin(hessian_out_t out, int length, char *type) {
	hessian_ensureCapacity(out, out->offset + 1);
	if (length < 0) {
		if (type != NULL) {
			hessian_write(out, (char) BC_LIST_VARIABLE);
			hessian_writeType(out, type);
		} else {
			hessian_write(out, (char) BC_LIST_VARIABLE_UNTYPED);
		}

		return true;
	} else if (length <= LIST_DIRECT_MAX) {
		if (type != NULL) {
			hessian_write(out, (char)(BC_LIST_DIRECT + length));
			hessian_writeType(out, type);
		} else {
			hessian_write(out, (char)(BC_LIST_DIRECT_UNTYPED + length));
		}

		return false;
	} else {
		if (type != NULL) {
			hessian_write(out, (char) BC_LIST_FIXED);
			hessian_writeType(out, type);
		} else {
			hessian_write(out, (char) BC_LIST_FIXED_UNTYPED);
		}

		hessian_writeInt(out, length);

		return false;
	}

	return 0;
}

int hessian_writeListEnd(hessian_out_t out) {
	hessian_ensureCapacity(out, out->offset + 1);
	hessian_write(out, (char) BC_END);

	return 0;
}

int hessian_writeType(hessian_out_t out, char *type) {
	int len = strlen(type);
	if (len == 0) {
		return 1;
	}

	// Do something with refs here
	return hessian_writeString(out, type);
}

int hessian_startCall(hessian_out_t out, char *method, int length) {
	hessian_ensureCapacity(out, out->offset + 1);
	hessian_write(out, 'C');

	hessian_writeString(out, method);
	hessian_writeInt(out, length);

	return 0;
}

int hessian_completeCall(hessian_out_t out) {
	return 0;
}

int hessian_printString(hessian_out_t out, char *value) {
	return hessian_printNString(out, value, 0, strlen(value));
}

int hessian_printNString(hessian_out_t out, char *value, int offset, int length) {
	hessian_ensureCapacity(out, out->offset + length);

	for (int i = 0; i < length; i++) {
		int ch = value[i + offset];

		if (ch < 0x80) {
			hessian_write(out, (int)(ch));
			out->length++;
		} else if (ch < 0x800) {
			hessian_write(out, (int)(0xc0 + ((ch >> 6) & 0x1f)));
			out->length++;
			hessian_write(out, (int)(0x80 + (ch & 0x3f)));
			out->length++;
		} else {
			hessian_write(out, (int)(0xe0 + ((ch >> 12) & 0xf)));
			out->length++;
			hessian_write(out, (int)(0x80 + ((ch >> 6) & 0x3f)));
			out->length++;
			hessian_write(out, (int)(0x80 + (ch & 0x3f)));
			out->length++;
		}
	}

	return 0;
}

void hessian_ensureCapacity(hessian_out_t obj, int capacity) {
	char *newArray;
	int oldCapacity;
	oldCapacity = obj->capacity;
	if (capacity > oldCapacity) {
		int newCapacity = (oldCapacity * 3) / 2 + 1;
		if (newCapacity < capacity) {
			newCapacity = capacity;
		}
		newArray = (char *) realloc(obj->buffer, sizeof(char *) * newCapacity);
		obj->capacity = newCapacity;
		obj->buffer = newArray;
	}
}

int hessian_write(hessian_out_t out, unsigned char byte) {
	out->buffer[out->offset++] = byte;
	out->length++;

	return 0;
}

int hessian_writeP(hessian_out_t out, unsigned char *byte) {
	out->buffer[out->offset++] = *byte;
	out->length++;

	return 0;
}
