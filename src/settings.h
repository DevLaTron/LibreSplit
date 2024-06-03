#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <jansson.h>

void get_libresplit_folder_path(char* out_path);
void ls_update_setting(const char* setting, json_t* value);
json_t* get_setting_value(const char* section, const char* setting);

#endif /* __SETTINGS_H__ */
