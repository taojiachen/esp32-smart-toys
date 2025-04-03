
void update_task(const char *json_string);
bool get_nearest_task(const int current_time, char **key, char **datavalue, int *starttime, int *keeptime);
void print_all_tasks(void);
void clear_all_tasks(void);
void get_endtime(char *endtime, const char *starttime, long keeptime);