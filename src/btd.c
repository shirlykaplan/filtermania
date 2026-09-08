
#define _CRT_SECURE_NO_WARNINGS

/***************
*              *
*   ANSWERS    *
*              *
****************/

/*
Moshe is a great magshimim student, he wrote some code but he had a lot of errors, can you help him?
when the code works enter the number: 10 and the numbers: 23,4,16,42,8,15,64,26,3,112
see what it prints...
*/


#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
int function1();

void function3(int *array_pointer, int *length);
/* DON'T TOUCH function4!!!!!!*/
void function4(int *array_pointer, int *length);
int function2(int* p_num);

int main()
{
	function1();
	system("Pause");
}

int function1()
{
	int* p_num = (int*)malloc(sizeof(int));
	if (p_num)
	{
		printf("Please enter a number : \n");
		scanf("%d", p_num);
		function2(p_num);
		free(p_num);
	}
	else
	{
		printf("Unsuccesful malloc!\n");
		return 0;
	}

}

int function2(int* p_num)
{
	int* array_pointer = (int*)malloc(sizeof(int)* (*p_num));
	int i;
	if (array_pointer)
	{
		for (i = 0; i < *(p_num); i++)
		{
			printf("Please enter a number :\n");
			scanf("%d", array_pointer + i);
		}
		function3(array_pointer, p_num);
		free(array_pointer);
	}
	else
	{
		printf("Unsuccesful malloc!\n");
		return 0;
	}


}
void function3(int *array_pointer, int *length)
{
	int i;
	int j = 0;
    int temp = 0; 
	for (i = 1; i <= *length - 1; i++)
	{
		j = i;
		while ((j > 0) && (*(array_pointer + j) < *(array_pointer + j - 1)))
		{
			temp = *(array_pointer + j);
			*(array_pointer + j) = *(array_pointer + j - 1);
			*(array_pointer + j - 1) = temp;
			j--;
		}
	}
	function4(array_pointer, length);
	printf("\n");

}

//DO NOT touch this function 
void function4(int *array_pointer, int *length)
{
	int i;
	char* very_interesting_string = (char*)malloc(sizeof(char)* 5);
	char buffer[3];

	*(very_interesting_string) = (char)(*(array_pointer)+63);
	*(very_interesting_string + 1) = (char)(*(array_pointer + 1) + 80);
	*(very_interesting_string + 2) = (char)(*(array_pointer + 2) + 60);
	*(very_interesting_string + 3) = (char)(*(array_pointer + 3) + 108);
	*(very_interesting_string + 4) = (char)(*(array_pointer + 9) + 13);

	for (i = 0; i < 4; i++)
	{
		printf("%c", *(very_interesting_string + i));
	}

	for (i = 4; i <= 8; i++)
	{
		_itoa(*(array_pointer + i), buffer, 10);
		*(very_interesting_string) = (*(array_pointer));
		printf("%s", buffer);
	}

	printf("%c", *(very_interesting_string + 4));

}