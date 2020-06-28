#include <stdio.h>
#include "smalloc.h"

int main()
{
	void *p1, *p2, *p3, *p4, *p5, *p6, *p7;

	print_sm_containers();

	p1 = smalloc(2000);
	printf("smalloc(2000):%p\n", p1);
	print_sm_containers();

	p2 = smalloc(500);
	printf("smalloc(500):%p\n", p2);
	print_sm_containers();

	p3 = smalloc(1400);
	printf("smalloc(1400):%p\n", p3);
	print_sm_containers();

	sfree(p3);
	printf("sfree(1400):%p\n", p3);
	print_sm_containers();

	p4 = smalloc(1000);
	printf("smalloc(1000):%p\n", p4);
	print_sm_containers();

	printf("srealloc(2000):%p\n", p4);
	srealloc(p4, 2000);
	print_sm_containers();

	p5 = smalloc(1200);
	printf("smalloc(1200):%p\n", p5);
	print_sm_containers();

	p6 = smalloc(1400);
	printf("smalloc(1400):%p\n", p5);
	print_sm_containers();

	printf("srealloc(2000):%p\n", p5);
	srealloc(p5, 2000);
	print_sm_containers();
	print_mem_uses();

	p7 = smalloc(500);
	printf("smalloc(500):%p\n", p5);
	print_sm_containers();

	sshrink();
	print_sm_containers();
	print_mem_uses();
}
