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
 * netstring.c
 *
 *  \date       Sep 13, 2013
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <apr_general.h>
#include <apr_strings.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "hash_map.h"
#include "netstring.h"
#include "utils.h"



int netstring_getNetStringLength(char* inNetStr)
{
	int posCnt = 0;
	int inNetStrLen = 0;

	while((posCnt < strlen(inNetStr)) && (isdigit(inNetStr[posCnt])))
	{
		inNetStrLen = (inNetStrLen*10) + (inNetStr[posCnt] - '0');
		posCnt++;
	}

	return inNetStrLen;
}

/**
 * in: "hallo"
 * out: "5:hallo,"
 */
celix_status_t netstring_encode(apr_pool_t *pool, char* inStr, char **outNetStr)
{
	celix_status_t status = CELIX_SUCCESS;

	if((inStr != NULL) && (strlen(inStr) > 0))
	{
		char* inStrLen = apr_itoa(pool, strlen(inStr));
		*outNetStr = apr_pstrcat(pool, inStrLen, ":", inStr, ",", NULL);
	}
	else
	{
		status = CELIX_BUNDLE_EXCEPTION;
	}

	return status;
}

/**
 * in: "5:hallo,"
 * out: "hallo"
 *
 */
celix_status_t netstring_decode(apr_pool_t *pool, char* inNetStr, char **outStr)
{
	celix_status_t status = CELIX_SUCCESS;

	if((inNetStr != NULL) && (strlen(inNetStr) > 0))
	{
		int inNetStrNumOfDigits = 0;
		int inNetStrLen = 0;

		// retrieve length of netstring
		inNetStrLen = netstring_getNetStringLength(inNetStr);
		inNetStrNumOfDigits = ceil(log10( (double) inNetStrLen + 1 ));

		if((inNetStrNumOfDigits == strlen(inNetStr)) || (inNetStr[inNetStrNumOfDigits] !=':') || ((inNetStrLen+inNetStrNumOfDigits+2) > strlen(inNetStr)))
		{
			status = CELIX_ILLEGAL_ARGUMENT;
		}
		else
		{
			*outStr = apr_pstrmemdup(pool, (inNetStr+inNetStrNumOfDigits+1), inNetStrLen);
		}
	}
	else
	{
		status = CELIX_BUNDLE_EXCEPTION;
	}

	return status;
}

celix_status_t netstring_custEncodeFromArray(apr_pool_t *pool, void *inStrArr[], customEncoderCallback inCallback, int inArrLen, char** outNetStr)
{
	celix_status_t status = CELIX_SUCCESS;

	int arrCnt = 0;
	char* totalOutStr = "";

	for(; (arrCnt < inArrLen) && (status == CELIX_SUCCESS); arrCnt++)
	{
		char* singleOutStr = NULL;

		if ((status = inCallback(pool, inStrArr[arrCnt], &singleOutStr)) == CELIX_SUCCESS)
		{
			totalOutStr = apr_pstrcat(pool, totalOutStr, singleOutStr, NULL);
		}
	}

	if (status == CELIX_SUCCESS)
	{
		*outNetStr = totalOutStr;
	}

	return status;
}


/**
 * in: {"hallo", "tom" }
 * out: "5:hallo,3:tom,"
  */

celix_status_t netstring_encodeFromArray(apr_pool_t *pool, char *inStrArr[], int inArrLen, char** outNetStr)
{
	return netstring_custEncodeFromArray(pool, (void**) inStrArr, (customEncoderCallback) netstring_encode, inArrLen, outNetStr);
}



celix_status_t netstring_custDecodeFromArray(apr_pool_t *pool, char* inNetStr, customDecoderCallback inCallback, void **outStrArr[], int *outArrSize)
{
	celix_status_t status = CELIX_SUCCESS;

	// get number of elements
	int netStrElementCnt = 0;
	int netStrElementLen = netstring_getNetStringLength(inNetStr);
	int netStrTotalLen = 0;
	int netStrNumOfDigits = 0;

	for(;netStrTotalLen < strlen(inNetStr) && netStrElementLen > 0; netStrElementCnt++)
	{
		netStrElementLen = netstring_getNetStringLength(inNetStr + netStrTotalLen);

		netStrNumOfDigits = ceil(log10( (double) netStrElementLen + 1 ));
		netStrTotalLen += netStrElementLen + netStrNumOfDigits + 2; /* colon and comma */;
	}

	// num of elements
	*outStrArr = apr_palloc(pool, netStrElementCnt * sizeof(*outStrArr));
	*outArrSize = netStrElementCnt;

	// assign
	for(netStrTotalLen = 0, netStrElementCnt = 0; (netStrTotalLen < strlen(inNetStr)) && (status == CELIX_SUCCESS); netStrElementCnt++)
	{
		netStrElementLen = netstring_getNetStringLength(inNetStr + netStrTotalLen);
		status = inCallback(pool, (inNetStr + netStrTotalLen), &(*outStrArr)[netStrElementCnt]);
		//status = netstring_decode(pool, (inNetStr + netStrTotalLen), &(*outStrArr)[netStrElementCnt]);
		netStrNumOfDigits = ceil(log10( (double) netStrElementLen + 1 ));
		netStrTotalLen += netStrElementLen + netStrNumOfDigits + 2; /* colon and comman */;
	}

	return status;
}


/**
 * in: "5:hallo,3:tom,"
 * out: {"hallo", "tom" }
  */
celix_status_t netstring_decodetoArray(apr_pool_t *pool, char* inNetStr, char **outStrArr[], int *outArrSize)
{

	return netstring_custDecodeFromArray(pool, inNetStr, (customDecoderCallback) netstring_decode, (void***) outStrArr, outArrSize);
}




celix_status_t netstring_encodeFromHashMap(apr_pool_t *pool, hash_map_pt inHashMap, char** outNetStr)
{
	celix_status_t status = CELIX_SUCCESS;

	hash_map_iterator_pt prpItr = hashMapIterator_create(inHashMap);
	char *completeEncKeyValStr = "";

	while((hashMapIterator_hasNext(prpItr) == true) && (status == CELIX_SUCCESS) )
	{
		hash_map_entry_pt prpEntry = hashMapIterator_nextEntry(prpItr);

		char *key = hashMapEntry_getKey(prpEntry);
		char *val = hashMapEntry_getValue(prpEntry);

		if ((key == NULL) || (val == NULL) )
		{
			status = CELIX_ILLEGAL_ARGUMENT;
		}
		else
		{
			char *keyValStr = apr_pstrcat(pool, "(key:", key, "=value:", val, ")", NULL);
			char *encKeyValStr = "";

			if ((status = netstring_encode(pool, keyValStr, &encKeyValStr)) == CELIX_SUCCESS)
			{
				completeEncKeyValStr = apr_pstrcat(pool, completeEncKeyValStr, encKeyValStr, NULL);
			}
		}
	}

	hashMapIterator_destroy(prpItr);

	*outNetStr = completeEncKeyValStr;

	return status;
}


celix_status_t netstring_custEncodeFromHashMap(apr_pool_t *pool, hash_map_pt inHashMap, customEncoderCallback keyCallback, customEncoderCallback valCallback, char** outNetStr)
{
	celix_status_t status = CELIX_SUCCESS;

	hash_map_iterator_pt prpItr = hashMapIterator_create(inHashMap);
	char *completeEncKeyValStr = "";

	while((hashMapIterator_hasNext(prpItr) == true) && (status == CELIX_SUCCESS) )
	{
		hash_map_entry_pt prpEntry = hashMapIterator_nextEntry(prpItr);

		void *keyPtr = hashMapEntry_getKey(prpEntry);
		char *key = keyPtr;
		void *valPtr = hashMapEntry_getValue(prpEntry);
		char *val = valPtr;

		if (keyCallback != NULL)
		{
			status = keyCallback(pool, keyPtr, &key);
		}

		if  (valCallback != NULL && status == CELIX_SUCCESS)
		{
			status = valCallback(pool, valPtr, &val);
		}

		if ((key == NULL) || (val == NULL))
		{
			status = CELIX_ILLEGAL_ARGUMENT;
		}
		else if (status == CELIX_SUCCESS)
		{
			char *keyValStr = apr_pstrcat(pool, "(key:", key, "=value:", val, ")", NULL);
			char *encKeyValStr = "";

			if ((status = netstring_encode(pool, keyValStr, &encKeyValStr)) == CELIX_SUCCESS)
			{
				completeEncKeyValStr = apr_pstrcat(pool, completeEncKeyValStr, encKeyValStr, NULL);
			}
		}
	}

	hashMapIterator_destroy(prpItr);

	*outNetStr = completeEncKeyValStr;

	return status;
}





celix_status_t netstring_decodeToHashMap(apr_pool_t *pool, char* inNetStr, hash_map_pt outHashMap)
{
	celix_status_t status = CELIX_SUCCESS;

	int netStrElementCnt = 0;
	int netStrElementLen = 0;
	int netStrTotalLen = 0;
	int netStrNumOfDigits = 0;

	if (inNetStr == NULL || outHashMap == NULL)
	{
		status = CELIX_ILLEGAL_ARGUMENT;
	}
	else
	{
		for(; (netStrTotalLen < strlen(inNetStr)) && (status == CELIX_SUCCESS); netStrElementCnt++)
		{
			char *singleString;

			netStrElementLen = netstring_getNetStringLength(inNetStr + netStrTotalLen); /* colon and comman */;
			status = netstring_decode(pool, (inNetStr + netStrTotalLen), &singleString);

			// contains key = value pair
			if ( (strncmp("(key:", singleString, 4) != 0) || (strstr(singleString, "=value:") == NULL) || (singleString[strlen(singleString)-1] != ')') )
			{
				status = CELIX_ILLEGAL_ARGUMENT;
			}
			else
			{
				char* index = strstr(singleString, "=value:");

				// copy from "(key:" to "=value:"
				char* key = apr_pstrndup(pool, singleString + 5, index - singleString - 5);
				char* val = apr_pstrndup(pool, index + 7, strlen(singleString) - strlen(key) - 13);

				hashMap_put(outHashMap, key, val);
			}

			netStrNumOfDigits = ceil(log10( (double) netStrElementLen + 1 ));
			netStrTotalLen += netStrElementLen + netStrNumOfDigits + 2;
		}
	}

	return status;
}




celix_status_t netstring_custDecodeToHashMap(apr_pool_t *pool, char* inNetStr, customDecoderCallback keyCallback, customDecoderCallback valCallback,  hash_map_pt outHashMap)
{
	celix_status_t status = CELIX_SUCCESS;

	int netStrElementCnt = 0;
	int netStrElementLen = 0;
	int netStrTotalLen = 0;
	int netStrNumOfDigits = 0;

	if (inNetStr == NULL || outHashMap == NULL)
	{
		status = CELIX_ILLEGAL_ARGUMENT;
	}
	else
	{
		for(; (netStrTotalLen < strlen(inNetStr)) && (status == CELIX_SUCCESS); netStrElementCnt++)
		{
			char *singleString;

			netStrElementLen = netstring_getNetStringLength(inNetStr + netStrTotalLen); /* colon and comman */;
			status = netstring_decode(pool, (inNetStr + netStrTotalLen), &singleString);

			// contains key = value pair
			if ( (strncmp("(key:", singleString, 4) != 0) || (strstr(singleString, "=value:") == NULL) || (singleString[strlen(singleString)-1] != ')') )
			{
				status = CELIX_ILLEGAL_ARGUMENT;
			}
			else
			{
				char* index = strstr(singleString, "=value:");

				// copy from "(key:" to "=value:"
				char* key = apr_pstrndup(pool, singleString + 5, index - singleString - 5);
				void* keyPtr = key;
				char* val = apr_pstrndup(pool, index + 7, strlen(singleString) - strlen(key) - 13);
				void* valPtr = val;

				if (keyCallback != NULL)
				{
					status = keyCallback(pool, key, &keyPtr);
				}

				if ((valCallback != NULL) && (status == CELIX_SUCCESS))
				{
					status = valCallback(pool, val, &valPtr);
				}

				if (status == CELIX_SUCCESS)
				{
					hashMap_put(outHashMap, keyPtr, valPtr);
				}
			}

			netStrNumOfDigits = ceil(log10( (double) netStrElementLen + 1 ));
			netStrTotalLen += netStrElementLen + netStrNumOfDigits + 2;
		}
	}

	return status;
}



celix_status_t netstring_encodeFromArrayList(apr_pool_t *pool, array_list_pt inList, char** outNetStr)
{
	return netstring_custEncodeFromArrayList(pool, inList, NULL, outNetStr);
}



celix_status_t netstring_custEncodeFromArrayList(apr_pool_t *pool, array_list_pt inList, customEncoderCallback encCallback, char** outNetStr)
{
	celix_status_t status = CELIX_SUCCESS;

	array_list_iterator_pt listItr = arrayListIterator_create(inList);
	char *completeEncListStr = "";

	while((arrayListIterator_hasNext(listItr) == true) && (status == CELIX_SUCCESS) )
	{
		void* listEntryPtr = arrayListIterator_next(listItr);
		char* listEntry = listEntryPtr;

		if (encCallback != NULL)
		{
			status = encCallback(pool, listEntryPtr, &listEntry);
		}

		if (listEntry == NULL)
		{
			status = CELIX_ILLEGAL_ARGUMENT;
		}
		else if (status == CELIX_SUCCESS)
		{
			char *listEntryStr = apr_pstrcat(pool, "(", listEntry, ")", NULL);
			char *encListEntryStr = NULL;

			if ((status = netstring_encode(pool, listEntryStr, &encListEntryStr)) == CELIX_SUCCESS)
			{
				completeEncListStr = apr_pstrcat(pool, completeEncListStr, encListEntryStr, NULL);
			}
		}
	}

	arrayListIterator_destroy(listItr);

	(*outNetStr) = completeEncListStr;

	return status;
}




celix_status_t netstring_decodeToArrayList(apr_pool_t *pool, char* inNetStr, array_list_pt outArrayList)
{
	return netstring_custDecodeToArrayList(pool, inNetStr, NULL, outArrayList);
}

celix_status_t netstring_custDecodeToArrayList(apr_pool_t *pool, char* inNetStr, customDecoderCallback decCallback, array_list_pt outArrayList)
{
	celix_status_t status = CELIX_SUCCESS;

	int netStrElementCnt = 0;
	int netStrElementLen = 0;
	int netStrTotalLen = 0;
	int netStrNumOfDigits = 0;

	if (inNetStr == NULL || outArrayList == NULL)
	{
		status = CELIX_ILLEGAL_ARGUMENT;
	}
	else
	{
		for(; (netStrTotalLen < strlen(inNetStr)) && (status == CELIX_SUCCESS); netStrElementCnt++)
		{
			char *singleString;

			netStrElementLen = netstring_getNetStringLength(inNetStr + netStrTotalLen); /* colon and comman */;
			status = netstring_decode(pool, (inNetStr + netStrTotalLen), &singleString);

			// check for braces
			if ( (singleString[0] != '(') || (singleString[strlen(singleString)-1] != ')') )
			{
				status = CELIX_ILLEGAL_ARGUMENT;
			}
			else
			{
				// copy without braces
				char* el = apr_pstrndup(pool, singleString + 1, strlen(singleString)-2);
				void* elPtr = el;

				if (decCallback != NULL)
				{
					status = decCallback(pool, el, &elPtr);
				}

				if (status == CELIX_SUCCESS)
				{
					arrayList_add(outArrayList, elPtr);
				}
			}

			netStrNumOfDigits = ceil(log10( (double) netStrElementLen + 1 ));
			netStrTotalLen += netStrElementLen + netStrNumOfDigits + 2;
		}
	}

	return status;
}
