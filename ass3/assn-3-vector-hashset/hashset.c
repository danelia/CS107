#include "hashset.h"


void HashSetNew(hashset *h, int elemSize, int numBuckets,
		HashSetHashFunction hashfn, HashSetCompareFunction comparefn, HashSetFreeFunction freefn)
{
	assert(elemSize > 0);
	assert(numBuckets > 0);
	assert(hashfn != NULL);
	assert(comparefn != NULL);
	
	h->numBuckets = numBuckets;
	h->freefn = freefn;
	h->comparefn = comparefn;
	h->hashfn = hashfn;
	h->logLen = 0;
	h->elemSize = elemSize;
	h->elems = malloc(numBuckets * sizeof(vector));
	
	assert(h->elems != NULL);
	
	for(int i = 0; i < numBuckets; i++)
		VectorNew((vector *)h->elems + i, elemSize, freefn, 0);
}

void HashSetDispose(hashset *h)
{
	assert(h != NULL);
	
	if(h->freefn != NULL)
		for(int i = 0; i < h->numBuckets; i++)
		{
			vector *v = (vector *)((char *)h->elems + i * sizeof(vector));
			VectorDispose(v);
			
			h->freefn((char *)h->elems + i * sizeof(vector));
		}
		
	free(h->elems);
}

int HashSetCount(const hashset *h)
{
	assert(h != NULL);
	return h->logLen;
}

void HashSetMap(hashset *h, HashSetMapFunction mapfn, void *auxData)
{
	assert(h != NULL);
	assert(mapfn != NULL);
	
	for(int i = 0; i < h->numBuckets; i++)
		VectorMap((vector *)h->elems + i , mapfn, auxData);
}

void HashSetEnter(hashset *h, const void *elemAddr)
{
	assert(h != NULL);
	assert(elemAddr != NULL);
	
	int hashCode = h->hashfn(elemAddr, h->numBuckets);
	assert(hashCode >= 0 && hashCode < h->numBuckets);
	
	vector *v = (vector *)h->elems + hashCode;
	
	int position = VectorSearch(v, elemAddr, h->comparefn, 0, false);
	
	if(position == -1)
	{
		VectorAppend(v, elemAddr);
		h->logLen++;
	}else
		VectorReplace(v, elemAddr, position);
}

void *HashSetLookup(const hashset *h, const void *elemAddr)
{
	assert(h != NULL);
	assert(elemAddr != NULL);
	
	int hashCode = h->hashfn(elemAddr, h->numBuckets);
	assert(hashCode >= 0 && hashCode < h->numBuckets);
	
	vector *v = (vector *)h->elems + hashCode;
	
	int position = VectorSearch(v, elemAddr, h->comparefn, 0, false);
	
	return position == -1 ? NULL : VectorNth(v, position);
}
