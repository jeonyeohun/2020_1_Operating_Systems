#include <unistd.h>
#include <stdio.h>
#include "smalloc.h"
#include <limits.h>
#include <string.h>

size_t total_size = 0;

sm_container_t sm_head = {
	0,
	&sm_head,
	&sm_head,
	0};

static void *
_data(sm_container_ptr e)
{
	return ((void *)e) + sizeof(sm_container_t);
}

static void
sm_container_split(sm_container_ptr hole, size_t size)
{
	sm_container_ptr remainder = (sm_container_ptr)(_data(hole) + size);

	remainder->dsize = hole->dsize - size - sizeof(sm_container_t);
	remainder->status = Unused;
	remainder->next = hole->next;
	remainder->prev = hole;
	hole->dsize = size;
	hole->next->prev = remainder;
	hole->next = remainder;
}

static void *
retain_more_memory(int size)
{
	sm_container_ptr hole;
	int pagesize = getpagesize();
	int n_pages = 0;

	n_pages = (sizeof(sm_container_t) + size + sizeof(sm_container_t)) / pagesize + 1;
	hole = (sm_container_ptr)sbrk(n_pages * pagesize);
	if (hole == 0x0)
		return 0x0;

	hole->status = Unused;
	hole->dsize = n_pages * getpagesize() - sizeof(sm_container_t);
	return hole;
}

void *
smalloc(size_t size)
{
	sm_container_ptr hole = 0x0, itr = 0x0;
	size_t min = INT_MAX;
	total_size += size + sizeof(sm_container_t);

	for (itr = sm_head.next; itr != &sm_head; itr = itr->next)
	{
		if (itr->status == Busy) // jump for using section
			continue;
		if ((itr->dsize == size) || (size + sizeof(sm_container_t) < itr->dsize))
		{ // when find empty section that fits or bigger than requested size
			if (itr->dsize == size)
			{
				hole = itr;
				break;
			}
			else
			{
				if (min > itr->dsize - (size + sizeof(sm_container_t)))
				{
					printf("%d\n", min);
					min = itr->dsize - (size + sizeof(sm_container_t));
					hole = itr;
				}
			}
		}
	}
	if (hole == 0x0)
	{
		hole = retain_more_memory(size);
		if (hole == 0x0)
			return 0x0;
		hole->next = &sm_head;
		hole->prev = sm_head.prev;
		(sm_head.prev)->next = hole;
		sm_head.prev = hole;
	}
	if (size < hole->dsize)
		sm_container_split(hole, size);
	hole->status = Busy;
	return _data(hole);
}

void sfree(void *p)
{
	sm_container_ptr itr;
	for (itr = sm_head.next; itr != &sm_head; itr = itr->next)
	{
		if (p == _data(itr))
		{
			sm_container_ptr start = itr;
			itr->status = Unused;
			if (itr->prev->status == Unused)
			{
				start = itr->prev;
				start->dsize += itr->dsize + sizeof(sm_container_t);
				start->next = itr->next;
				itr->next->prev = start;
			}

			if (start->next->status == Unused)
			{
				start->dsize += start->next->dsize + sizeof(sm_container_t);
				start->next = start->next->next;
				start->next->next->prev = start;
			}
			break;
		}
	}
}
void print_mem_uses()
{
	sm_container_ptr itr;
	size_t current_alloc_total = 0;
	size_t current_free_total = 0;
	for (itr = sm_head.next; itr != &sm_head; itr = itr->next)
	{
		if (itr->status == Busy)
			current_alloc_total += itr->dsize + sizeof(sm_container_t);
		else
			current_free_total += itr->dsize + sizeof(sm_container_t);
	}

	fprintf(stderr, "============== memory usage information =============\n");
	fprintf(stderr, "Total memory retained: %zu\n", total_size);
	fprintf(stderr, "Current allocated space: %zu\n", current_alloc_total);
	fprintf(stderr, "Current free space: %zu\n", current_free_total);
};
void *srealloc(void *p, size_t newsize)
{
	sm_container_ptr itr, hole;
	size_t temp_size = 0;
	for (itr = sm_head.next; itr != &sm_head; itr = itr->next)
	{
		if (p == _data(itr))
		{
			// case 1: If it is not possible to extend from current memory location
			if (itr->next == &sm_head || itr->next->status != Unused || itr->next->dsize + sizeof(sm_container_t) < newsize - itr->dsize)
			{
				// case 1-1: If there is possible hole to migrate target
				for (hole = sm_head.next; hole != &sm_head; hole = hole->next)
				{
					if (hole->status == Unused && hole->dsize >= newsize)
					{
						sm_container_split(hole, newsize);
						hole->status = Busy;
						memcpy(hole, p, newsize);
						sfree(p);
						return _data(hole);
					}
				}

				// case 1-2: If there is no possible hole
				sm_container_ptr temp = smalloc(newsize);
				memcpy(temp, p, newsize);
				sfree(p);

				return _data(temp);
			}
			// case 2: If there is a hole in contiguous memory space from the target
			else if (itr->next->status == Unused && itr->next->dsize + sizeof(sm_container_t) >= newsize - itr->dsize)
			{
				itr->next->dsize -= newsize - itr->dsize;
				itr->dsize = newsize;
			}
		}
	}
	return _data(itr);
}
void sshrink()
{
	size_t current_free_total = 0;
	sm_container_ptr itr;
	for (itr = sm_head.next; itr != &sm_head; itr = itr->next)
	{
		if (itr->status != Busy)
		{
			itr->prev->next = itr->next;
			itr->next->prev = itr->prev;
			current_free_total += itr->dsize + sizeof(sm_container_t);
		}

		sbrk(current_free_total * -1);
	}
}

void print_sm_containers()
{
	sm_container_ptr itr;
	int i;

	printf("==================== sm_containers ====================\n");
	for (itr = sm_head.next, i = 0; itr != &sm_head; itr = itr->next, i++)
	{
		printf("%3d:%p:%s:", i, _data(itr), itr->status == Unused ? "Unused" : "  Busy");
		printf("%8d:", (int)itr->dsize);

		int j;
		char *s = (char *)_data(itr);
		for (j = 0; j < (itr->dsize >= 8 ? 8 : itr->dsize); j++)
			printf("%02x ", s[j]);
		printf("\n");
	}
	printf("\n");
}
