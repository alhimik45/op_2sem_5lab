/* Обработать запросы к расписанию самолётов */
#include <malloc.h>
#include <string.h>
#include <locale.h>
#include <stdint.h>
#include <ncursesw/curses.h>
#include <ncursesw/menu.h>
#include <ncursesw/panel.h>
#include <ncursesw/form.h>
#include "flight.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define MENU_PAGE_ITEMS 16 // маскимальное количество отображаемых элементов меню
#define ESCAPE 27

typedef enum { in_main, in_form, some_error } program_state;
static flight_table table;

WINDOW * err_win;
PANEL * err_panel;
ITEM **items;
flight* filter_query;

/* Вычисление координат для рисования в центре */
int centered(int parent_dimension, int child_dimension);
/* Инициализация всего, что нужно */
void init_all();
/* Освобождение всего, что нужно */
void free_all();
/* Удаление записи из таблицы */
void del_flight(int id);
/* Проверка, подходит ли запись под запрос */
char fits_query(flight* f, flight* query);
/* Создаёт окно главного меню. */
int show_main_menu();
/* Создание окна, в котором будет показываться ошибка */
void create_error_message_window();
/* Показывает форму ввода данных рейса */
flight* get_form_data(flight*, char);
/* Показ окна с сообщением об ошибке */
void show_error_message(char*);
/* Печать инструкций управления и информации о записях */
void print_info(program_state state);
/* Обновление экрана */
void update_view(program_state state);

/* Вычисление координат для рисования в центре */
int centered(int parent_dimension, int child_dimension){
	return (parent_dimension - child_dimension)/2;
}

/* Инициализация всего, что нужно */
void init_all(){
	setlocale(LC_ALL,"");
	initscr();
	start_color();
	//use_default_colors();
	init_pair(1, COLOR_YELLOW, COLOR_BLACK);
	init_pair(2, COLOR_BLACK, COLOR_RED);
	init_pair(3, COLOR_WHITE, COLOR_BLUE);
	cbreak();
	noecho();
	curs_set(0);
	keypad(stdscr, TRUE);
	create_error_message_window();
	table.size = 0;
	filter_query = NULL;
}

/* Освобождение всего, что нужно */
void free_all(){
	int i;
	for(i = 0; i < table.size; ++i){
		free(table.arr[i]);
	}
	endwin();
}

/* Удаление записи из таблицы */
void del_flight(int id){
	int i;
	free(table.arr[id]);
	for(i = id; i < table.size - 1; ++i){
		table.arr[i] = table.arr[i+1];
	}
	table.size--;
}

/* Проверка, подходит ли запись под запрос */
char fits_query(flight* f, flight* query){
	const char empty_field_str[] = "                "; // FORM_STR_LEN  пробелов
	char result = 1;
	if(query != NULL){
		if(query->number != 0)
			result = result && f->number == query->number;
		if(strcmp(query->airplane_name, empty_field_str) != 0)
			result = result && strcmp(f->airplane_name, query->airplane_name) == 0;
		if(strcmp(query->city_name, empty_field_str) != 0)
			result = result && strcmp(f->city_name, query->city_name) == 0;
		if(query->days != 0)
			result = result && (f->days & query->days);
		if(query->time_from.hour > -1)
			result = result && timecmp(query->time_from, f->time_from) >= 0;
		if(query->time_to.hour > -1)
			result = result && timecmp(query->time_to, f->time_to) >= 0;
		if(query->cost != 0)
			result = result && f->cost <= query->cost;
	}
	return result;
}

/* Создаёт окно главного меню */
int show_main_menu(){
	WINDOW * main_menu_win;
	MENU *main_menu;
	PANEL  *main_menu_panel;
	int window_width = COLS/3*2;
	int c, real_menu_size=0;
	intptr_t i;
	char* caption = "Расписание самолётов";
	/* Создание меню */
	items = malloc(sizeof(ITEM*)*table.size+1);
	for(i = 0; i < table.size; ++i){
		if(fits_query(table.arr[i], filter_query)){
			items[real_menu_size] = new_item(flight_to_string(table.arr[i]), NULL);
			set_item_userptr(items[real_menu_size], (void*)i);
			real_menu_size++;
		}
	}	
	items[real_menu_size] = new_item(NULL, NULL);
	main_menu = new_menu((ITEM **)items);
	/* Создание окна для меню */
	int window_height =  MENU_PAGE_ITEMS + 7; // 7 = 4 рамки + 1 заголовок + 1 подпись элементов + 1 отступ для красоты
	main_menu_win = newwin(window_height, window_width+2, centered(LINES, window_height), centered(COLS, window_width+2)); // +2 т.к рамки
	keypad(main_menu_win, TRUE);
	set_menu_win(main_menu, main_menu_win);
	set_menu_sub(main_menu, derwin(main_menu_win, MENU_PAGE_ITEMS, window_width, 6, 1));
	main_menu_panel = new_panel(main_menu_win);
	/* Указатель на текущий пункт */
	set_menu_mark(main_menu, " -> ");
	/* Рисуем рамку */
	box(main_menu_win, 0, 0);
    /* Печать заголовка и подписи */
	wattron(main_menu_win, COLOR_PAIR(1));
	mvwprintw(main_menu_win, 1, centered(window_width, strlen(caption)/2), "%s", caption);
	wattroff(main_menu_win, COLOR_PAIR(1));
	mvwhline(main_menu_win, 2, 1, ACS_HLINE, window_width);
	mvwprintw(main_menu_win, 3, 5, "№         Самолёт             Город               Дни                 Время вылета  Время прилёта  Цена");
	mvwhline(main_menu_win, 4, 1, ACS_HLINE, window_width);
	post_menu(main_menu);
	wrefresh(main_menu_win);
	update_view(in_main);
	intptr_t selected = 0;
	char cycle = 1;
	flight* record;
	while(cycle && (c = wgetch(main_menu_win)) != ESCAPE){
		switch(c)
		{	case KEY_DOWN:
			if(menu_driver(main_menu, REQ_DOWN_ITEM)== E_OK)
				++selected;
			break;
			case KEY_UP:
			if(menu_driver(main_menu, REQ_UP_ITEM)== E_OK)
				--selected;
			break;
			case 'a': case 'A':
			hide_panel(main_menu_panel);
			record = get_form_data(NULL, 0);
			if(record){
				table.arr[table.size++] = record;
				cycle = 0;
			}	
			top_panel(main_menu_panel);		
			break;
			case 'd': case 'D':
			if(table.size == 0)
				show_error_message("Нечего удалять");
			else {
				del_flight((intptr_t)item_userptr(items[selected]));
				cycle = 0;
			}
			break;
			case 'e': case 'E':
			if(table.size == 0)
				show_error_message("Нечего редактировать");
			else {
				hide_panel(main_menu_panel);
				i = (intptr_t)item_userptr(items[selected]);
				record = get_form_data(table.arr[i], 0);
				if(record){
					free(table.arr[i]);
					table.arr[i] = record;
					cycle = 0;
				}
				top_panel(main_menu_panel);
			}
			break;
			case 'q': case 'Q':
			if(table.size == 0)
				show_error_message("Нет записей для выполнения запроса");
			else {
				hide_panel(main_menu_panel);
				if(filter_query)
					free(filter_query);
				filter_query = get_form_data(NULL, 1);
				cycle = !filter_query;

			}
			top_panel(main_menu_panel);	
			break;
			case 'c': case 'C':
			if(filter_query)
				free(filter_query);
			filter_query = NULL;
			cycle = 0;
			break;
		}

		update_view(in_main);
	}
	/* Освобождение памяти */
	unpost_menu(main_menu);
	free_menu(main_menu);
	for(i = 0; i < real_menu_size; ++i)
		free_item(items[i]);
	free(items);
	delwin(main_menu_win);
	del_panel(main_menu_panel);
	return c;
}

/* Создание окна, в котором будет показываться ошибка */
void create_error_message_window(){
	int window_width = COLS/4;
	int window_height =  5; // 5 = 2 рамки + 1 сообщение + 1 отступ + 1 псевдо-кнопка ОК
	err_win = newwin(window_height, window_width, centered(LINES, window_height), centered(COLS, window_width));
	keypad(err_win, TRUE);
	err_panel = new_panel(err_win);
}

/* Показывает форму ввода данных рейса */
flight* get_form_data(flight* editable, char query){
	FIELD *field[15]; // 15 = 7 полей + 7 подписей + 1 NULL-панель
	FORM  *form;
	WINDOW *form_win;
	PANEL * form_panel;
	flight* result = NULL;
	int ch, rows, cols;
	unsigned int i;
	const char* caption = "Рейс самолёта";
	const char* labels[] = {
		"Номер",
		"Самолёт",
		"Город",
		"Дни",
		"Время вылета",
		"Время прилёта",
		"Цена"
	};
	const char normal_number[] = "^[1-9][0-9]{0,8} *$",
				 normal_string[] = "^[[:alnum:]]+ *$",
				 days_enum[] = "^([1-7],)*[1-7] *$",
				 time_string[] = "^([0-1]?[0-9]|2[0-3])\\.[0-5]?[0-9] *$";
	curs_set(1);
	/* Установка полей */
	for(i = 0; i < ARRAY_SIZE(labels)*2; i += 2){
		field[i] = new_field(1, FORM_STR_LEN, i+2, 0, 0, 0);
		field[i+1] = new_field(1, FORM_STR_LEN, i+1, 0, 0, 0);
		set_field_buffer(field[i+1], 0, labels[i/2]);
		field_opts_off(field[i+1], O_ACTIVE);
		set_field_back(field[i], COLOR_PAIR(3));
		if(!query){
			field_opts_off(field[i], O_NULLOK);
			field_opts_off(field[i], O_PASSOK);
		}
		field_opts_off(field[i], O_AUTOSKIP);
	}
	field[i] = NULL;
	if(editable){
		char * buff = malloc(sizeof(char) * (FORM_STR_LEN+1));
		sprintf(buff, "%d", editable->number);
		set_field_buffer(field[0], 0, buff);

		buff = malloc(sizeof(char) * UTF_FORM_STR_LEN);
		strcpy(buff, editable->airplane_name);
		set_field_buffer(field[2], 0, buff);

		buff = malloc(sizeof(char) * UTF_FORM_STR_LEN);
		strcpy(buff, editable->city_name);
		set_field_buffer(field[4], 0, buff);

		buff = malloc(sizeof(char) * (FORM_STR_LEN+1));
		days_to_string(buff, editable->days);
		set_field_buffer(field[6], 0, buff);

		buff = malloc(sizeof(char) * TIME_STR_LEN);
		time_to_string(buff, editable->time_from);
		set_field_buffer(field[8], 0, buff);

		buff = malloc(sizeof(char) * TIME_STR_LEN);
		time_to_string(buff, editable->time_to);
		set_field_buffer(field[10], 0, buff);

		buff = malloc(sizeof(char) * (FORM_STR_LEN+1));
		sprintf(buff, "%d", editable->cost);
		set_field_buffer(field[12], 0, buff);
	}
	set_field_type(field[0], TYPE_REGEXP, normal_number); // Номер только число
	set_field_type(field[2], TYPE_REGEXP, normal_string); // Название самолёта - хотя бы один символ
	set_field_type(field[4], TYPE_REGEXP, normal_string); // Название города - хотя бы один символ
	set_field_type(field[6], TYPE_REGEXP, days_enum); // Дни - числа от 1 до 7 через запятую
	set_field_type(field[8], TYPE_REGEXP, time_string); // Время
	set_field_type(field[10], TYPE_REGEXP, time_string); // Время
	set_field_type(field[12], TYPE_REGEXP, normal_number); // Цена только число
	form = new_form(field);
	set_form_opts(form, O_NL_OVERLOAD);
	scale_form(form, &rows, &cols);
	/* Создание окна для формы */
	form_win = newwin(rows + 4, cols + 4, centered(LINES, rows), centered(COLS, cols));
	keypad(form_win, TRUE);
	set_form_win(form, form_win);
	set_form_sub(form, derwin(form_win, rows, cols, 2, 2));
	box(form_win, 0, 0);
	form_panel = new_panel(form_win);
    /* Заголовок */
	wattron(form_win, COLOR_PAIR(1));
	mvwprintw(form_win, 1, centered(cols, strlen(caption)/2), "%s", caption);
	wattroff(form_win, COLOR_PAIR(1));
	post_form(form);
	update_view(in_form);
	form_driver(form, REQ_END_LINE);
	char cycle = 1;
	char valid = 1;
	while(cycle && (ch = wgetch(form_win)) != ESCAPE){
		switch(ch){
			case KEY_DOWN:
			if(form_driver(form, REQ_NEXT_FIELD) == E_INVALID_FIELD){
				show_error_message("Неправильно заполнено поле");
			}
			form_driver(form, REQ_END_LINE);
			break;
			case KEY_UP:
			if(form_driver(form, REQ_PREV_FIELD) == E_INVALID_FIELD){
				show_error_message("Неправильно заполнено поле");
			}
			form_driver(form, REQ_END_LINE);
			break;
			case KEY_BACKSPACE:
			form_driver(form, REQ_DEL_PREV);
			break;
			case 10:
			valid = 1;
			for(i = 0; i < 7 && valid; ++i){
				if(form_driver(form, REQ_NEXT_FIELD) == E_INVALID_FIELD){
					show_error_message("Неправильно заполнено поле");
					valid = 0;
				}
			}
			if(valid){
				result = make_flight(field_buffer(field[0],0),
					field_buffer(field[2],0),
					field_buffer(field[4],0),
					field_buffer(field[6],0),
					field_buffer(field[8],0),
					field_buffer(field[10],0),
					field_buffer(field[12],0));
				cycle = 0;
			}
			break;
			default:
			form_driver(form, ch);
			break;
		}
		update_view(in_form);
	}
	unpost_form(form);
	free_form(form);
	for(i = 0; i < ARRAY_SIZE(labels)*2; ++i){
		free_field(field[i]);
	}
	delwin(form_win);
	del_panel(form_panel);
	curs_set(0);
	return result;
}

/* Показ окна с сообщением об ошибке */
void show_error_message(char* msg){
	int width, height;
	getmaxyx(err_win,height, width);
	werase(err_win);
	/* Рисуем рамку */
	box(err_win, 0, 0);
    /* Цвет фона */
	wbkgd(err_win,COLOR_PAIR(2));
    /* Печать сообщения */
	wattron(err_win, COLOR_PAIR(2));
	mvwprintw(err_win, 1, centered(width, strlen(msg)/2), "%s", msg);
	wattroff(err_win, COLOR_PAIR(2));
	wattron(err_win, COLOR_PAIR(3));
    /* Печать псевдо-кнопки */
	mvwprintw(err_win, 3, centered(width, 4), "%s", " OK ");
	wattroff(err_win, COLOR_PAIR(3));
	top_panel(err_panel);
	update_view(some_error);
	while(wgetch(err_win) != 10);
	hide_panel(err_panel);
}

/* Печать инструкций управления и информации о записях */
void print_info(program_state state){
	mvprintw(LINES - 3, 0, "Записей на текущий момент: %d", table.size);
	switch(state){
		case in_main:
		mvprintw(LINES - 2, 0, "Стрелки - выбирать пункты, A - добавить запись, E - редактировать запись, D - удалить запись, Q - сделать запрос, C - очистить текущий запрос, Esc - выход");
		break;
		case in_form: 
		mvprintw(LINES - 2, 0, "Стрелки - выбирать поля, Enter - закончить ввод, Esc - отмена");
		break;
		case some_error:
		mvprintw(LINES - 2, 0, "Нажмите Enter для продолжения");
		break;
	}
}

/* Обновление экрана */
void update_view(program_state state){
	clear();
	print_info(state);
	update_panels();
	doupdate();
}

int main(){
	init_all();
	while(show_main_menu() != ESCAPE);
	free_all();
	return 0;
}