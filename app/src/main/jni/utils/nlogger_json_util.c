

#include <stdlib.h>
#include <string.h>
#include "nlogger_json_util.h"

json_map_nlogger *create_json_map_nlogger(void) {
    json_map_nlogger *item = malloc(sizeof(json_map_nlogger));
    if (NULL != item)
        memset(item, 0, sizeof(json_map_nlogger));
    return item;
}

int is_empty_json_map_nlogger(json_map_nlogger *item) {
    json_map_nlogger temp;
    memset(&temp, 0, sizeof(json_map_nlogger));
    if (memcmp(item, &temp, sizeof(json_map_nlogger)) == 0) {
        return 1;
    }
    return 0;
}

void add_item_string_nlogger(json_map_nlogger *map, const char *key, const char *value) {
    if (NULL != map && NULL != value && NULL != key && strnlen(key, 128) > 0) {
        json_map_nlogger *item = map;
        json_map_nlogger *temp = item;
        if (!is_empty_json_map_nlogger(item)) {
            while (NULL != item->next) {
                item = item->next;
            }
            temp = create_json_map_nlogger();
            item->next = temp;
        }
        if (NULL != temp) {
            temp->type = NLOGGER_JSON_MAP_STRING;
            temp->key = (char *) key;
            temp->value_str = value;
        }
    }
}

void add_item_number_nlogger(json_map_nlogger *map, const char *key, double number) {
    if (NULL != map && NULL != key && strnlen(key, 128) > 0) {
        json_map_nlogger *item = map;
        json_map_nlogger *temp = item;
        if (!is_empty_json_map_nlogger(item)) {
            while (NULL != item->next) {
                item = item->next;
            }
            temp = create_json_map_nlogger();
            item->next = temp;
        }
        if (NULL != temp) {
            temp->type = NLOGGER_JSON_MAP_NUMBER;
            temp->key = (char *) key;
            temp->value_num = number;
        }
    }
}

void add_item_bool_nlogger(json_map_nlogger *map, const char *key, int boolValue) {
    if (NULL != map && NULL != key && strnlen(key, 128) > 0) {
        json_map_nlogger *item = map;
        json_map_nlogger *temp = item;
        if (!is_empty_json_map_nlogger(item)) {
            while (NULL != item->next) {
                item = item->next;
            }
            temp = create_json_map_nlogger();
            item->next = temp;
        }
        if (NULL != temp) {
            temp->type = NLOGGER_JSON_MAP_BOOL;
            temp->key = (char *) key;
            temp->value_bool = boolValue;
        }
    }
}

void delete_json_map_nlogger(json_map_nlogger *map) {
    if (NULL != map) {
        json_map_nlogger *item = map;
        json_map_nlogger *temp = NULL;
        do {
            temp = item->next;
            free(item);
            item = temp;
        } while (NULL != item);
    }
}

void inflate_json_by_map_nlogger(cJSON *root, json_map_nlogger *map) {
    if (NULL != root && NULL != map) {
        json_map_nlogger *item = map;
        do {
            switch (item->type) {
                case NLOGGER_JSON_MAP_STRING:
                    if (NULL != item->value_str) {
                        cJSON_AddStringToObject(root, item->key, item->value_str);
                    }
                    break;
                case NLOGGER_JSON_MAP_NUMBER:
                    cJSON_AddNumberToObject(root, item->key, item->value_num);
                    break;
                case NLOGGER_JSON_MAP_BOOL:
                    cJSON_AddBoolToObject(root, item->key, item->value_bool);
                    break;
                default:
                    break;
            }
            item = item->next;
        } while (NULL != item);
    }
}
