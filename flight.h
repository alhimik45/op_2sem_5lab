#ifndef APP_FLIGHT_HEADER
#define APP_FLIGHT_HEADER

#define FORM_STR_LEN 16 // длина строки формы (без концевого нуля)
#define UTF_FORM_STR_LEN (FORM_STR_LEN * 2 + 1) // длина строки формы в utf (с концевым нулём)
#define STRING_REPR_LEN 110 // длина строкового представления рейса в wide char
#define UTF_STRING_REPR_LEN (STRING_REPR_LEN * 2 + 1) // длина строкового представления рейса в char
#define TIME_STR_LEN 6 // длина строкового представления времени с концевым нулём

typedef struct {
	int hour;
	int minute;
} flight_time;

typedef struct {
	int number;
	char airplane_name[UTF_FORM_STR_LEN];  // умножаем на 2 т.к юникод
	char city_name[UTF_FORM_STR_LEN]; // умножаем на 2 т.к юникод
	char days;
	flight_time time_from;
	flight_time time_to;
	int cost;
	char string_repr[UTF_STRING_REPR_LEN];
} flight;

typedef struct {
	flight* arr[100];
	int size;
} flight_table;

/* Конвертирует битовую маску, используемую для компактного хранения дней в понятное строковое представление */
void days_to_string(char* s, char days);
/* Конвертирует строку с перечислением дней в битовую маску */
char string_to_days(char* str);
/* Конвертирует строку в структуру flight_time, используемую для внутрененго хранения времени */
flight_time string_to_time(char* str);
/* Конветрирует внутреннее представление времени в понятную строку */
void time_to_string (char* s, flight_time t);
/* Сравнивает два времени, если первое больше - возвращает положительное число, если равны - 0, если первое меньше - отрицательное число */
char timecmp(flight_time t1, flight_time t2);
/* Конвертирует структуру рейса в строку */
char* flight_to_string(flight* f);
/* Создаёт структуру рейса из строк */
flight* make_flight(char* number_str, 
					char* airplane_name, 
					char* city_name,
					char* days,
					char* time_from,
					char* time_to,
					char* cost);
#endif