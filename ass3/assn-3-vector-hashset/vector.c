#include "vector.h"
#include "search.h"


static void Grow(vector *v)
{
	v->allocLen += v->initialAlloc;
	v->elems = realloc(v->elems, v->allocLen * v->elemSize);
	assert(v->elems != NULL);
}

void VectorNew(vector *v, int elemSize, VectorFreeFunction freeFn, int initialAllocation)
{
	assert(elemSize > 0);
	
	v->elemSize = elemSize;
	v->logLen = 0;
	v->allocLen = (initialAllocation == 0) ? 4 : initialAllocation;
	v->freeFn = freeFn;
	v->initialAlloc = v->allocLen;
	v->elems = malloc(v->allocLen * v->elemSize);
	assert(v->elems != NULL);
}

void VectorDispose(vector *v)
{
	assert(v != NULL);
	if(v->freeFn != NULL)
		for(int i = 0; i < v->logLen; i++)
			v->freeFn(VectorNth(v, i));
	
	free(v->elems);
}

int VectorLength(const vector *v)
{
	assert(v != NULL);
	return v->logLen;
}

void *VectorNth(const vector *v, int position)
{
	assert(v != NULL);
	assert(position < v->logLen  && position >= 0);
	return (char *)v->elems + v->elemSize * position;
}

void VectorReplace(vector *v, const void *elemAddr, int position)
{
	assert(v != NULL);
	assert(position < v->logLen && position >= 0);

	if(v->freeFn != NULL)
		v->freeFn((char *)v->elems + v->elemSize * position);
	
	memcpy((char *)v->elems + v->elemSize * position, elemAddr, v->elemSize);
}

void VectorInsert(vector *v, const void *elemAddr, int position)
{
	assert(v != NULL);
	assert(position <= v->logLen && position >= 0);
	
	if(v->logLen == v->allocLen)
		Grow(v);
	
	void *source = (char *)v->elems + position * v->elemSize;
	
	memmove((char *)source + v->elemSize, source, (v->logLen - position) * v->elemSize);
	
	memcpy(source, elemAddr, v->elemSize);
	v->logLen++;
}

void VectorAppend(vector *v, const void *elemAddr)
{
	VectorInsert(v, elemAddr, v->logLen);
}

void VectorDelete(vector *v, int position)
{
	assert(v != NULL);
	assert(position < v->logLen && position >= 0);
	
	void *source = (char *)v->elems + position * v->elemSize;
	
	if(v->freeFn != NULL)
		v->freeFn(source);
	
	v->logLen--;
	memmove(source, (char *)source + v->elemSize, (v->logLen - position) * v->elemSize);
	
}

void VectorSort(vector *v, VectorCompareFunction compare)
{
	assert(v != NULL);
	assert(compare != NULL);
	
	qsort(v->elems, v->logLen, v->elemSize, compare);
}

void VectorMap(vector *v, VectorMapFunction mapFn, void *auxData)
{
	assert(v != NULL);
	assert(mapFn != NULL);
	
	for(int i = 0; i < v->logLen; i++)
		mapFn((char *)v->elems + i * v->elemSize, auxData);
}

static const int kNotFound = -1;
int VectorSearch(const vector *v, const void *key, VectorCompareFunction searchFn, int startIndex, bool isSorted)
{
	assert(v != NULL);
	assert(searchFn != NULL);
	assert(key != NULL);
	assert(startIndex <= v->logLen && startIndex >=0);
	
	if(v->logLen == 0)
		return kNotFound;
	
	size_t numOfElems = v->logLen - startIndex;
	void * found;
	if(isSorted)
		found = bsearch(key, (char *)v->elems + v->elemSize * startIndex, numOfElems, v->elemSize, searchFn);
	else
		found = lfind(key, (char *)v->elems + v->elemSize * startIndex, &numOfElems, v->elemSize, searchFn);
	
	if(found != NULL)
			return ((char *)found - (char *)v->elems)/v->elemSize;
	
	return kNotFound;
}