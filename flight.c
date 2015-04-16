#include "flight.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <wchar.h>

/* Конвертирует битовую маску, используемую для компактного хранения дней в понятное строковое представление */
void days_to_string(char* s, char days){
	char writed = 0;
	int i;
	for(i = 0; i < 7;++i){
		if (days & 1<<i){
			s+=sprintf(s,"%d,", i+1);
			writed = 1;
		}
	}
	if(writed) //удаление лишней запятой
		s[-1]='\0';
}

/* Конвертирует строку с перечислением дней в битовую маску */
char string_to_days(char* str){
	char days = 0;
	char c;
	if(*str != ' '){
		while(*str != '\0' && *str != ' '){
			c = *(str+1);
			*(str+1) = '\0';
			days = days | 1<<(atoi(str)-1);
			*(str+1) = c;
			str++;
			if (*str != '\0' && *str != ' '){
				str++;
			}
		}
	}
	return days;
}

/* Конвертирует строку в структуру flight_time, используемую для внутрененго хранения времени */
flight_time string_to_time(char* str){
	flight_time t;
	char* delimiter_pos = strstr(str,".");
	if(delimiter_pos == NULL){
		t.hour = -1;
		t.minute = -1;
	}else {
		*delimiter_pos ='\0';
		t.hour = atoi(str);
		*delimiter_pos ='.';
		t.minute = atoi(delimiter_pos+1);
	}
	return t;
}

/* Конветрирует внутреннее представление времени в понятную строку */
void time_to_string (char* s, flight_time t){
	sprintf(s, "%02d.%02d", t.hour, t.minute);
}

/* Сравнивает два времени, если первое больше - возвращает положительное число, если равны - 0, если первое меньше - отрицательное число */
char timecmp(flight_time t1, flight_time t2){
	if (t1.hour == t2.hour){
		return t1.minute - t2.minute;
	}else {
		return t1.hour - t2.hour;
	}
}

/* Конвертирует структуру рейса в строку */
char* flight_to_string(flight* f){
	if(f->string_repr[0]!='\0')
		return f->string_repr;
	char days_str[FORM_STR_LEN];
	char time_from_str[TIME_STR_LEN], time_to_str[TIME_STR_LEN];
	int pos;
	wchar_t utf_airplane_name[UTF_FORM_STR_LEN];
	wchar_t utf_city_name[UTF_FORM_STR_LEN];
	wchar_t buffer[STRING_REPR_LEN + 1];
	pos = swprintf(utf_airplane_name, UTF_FORM_STR_LEN, L"%hs", f->airplane_name);
	utf_airplane_name[pos] = '\0';
	pos = swprintf(utf_city_name, UTF_FORM_STR_LEN, L"%hs", f->city_name);
	utf_city_name[pos] = '\0';
	days_to_string(days_str, f->days);
	time_to_string(time_from_str, f->time_from);
	time_to_string(time_to_str, f->time_to);
	swprintf(buffer, STRING_REPR_LEN + 1, L"%-10d%-20ls%-20ls%-20s%s         %s         %d", 
		f->number, 
		utf_airplane_name,
		utf_city_name,
		days_str,
		time_from_str,
		time_to_str,
		f->cost);
	wcstombs(f->string_repr, buffer, UTF_STRING_REPR_LEN);
	return f->string_repr;
}

/* Создаёт структуру рейса из строк */
flight* make_flight(char* number_str, 
					char* airplane_name, 
					char* city_name,
					char* days,
					char* time_from,
					char* time_to,
					char* cost)
{
	flight* f = malloc(sizeof(flight));
	f->number = atoi(number_str);	
	strcpy(f->airplane_name, airplane_name);
	strcpy(f->city_name, city_name);
	f->days = string_to_days(days);
	f->time_from = string_to_time(time_from);
	f->time_to = string_to_time(time_to);
	f->cost = atoi(cost);
	f->string_repr[0]='\0';
	return f;
}