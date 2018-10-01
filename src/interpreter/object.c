//
// Created by onur on 11.07.2018.
//

#ifndef ZEROSCRIPT_OBJECT_C
#define ZEROSCRIPT_OBJECT_C

#include "object.h"

Z_INLINE z_object_t *string_new(char *data);

void load_class_code(const char *class_name, char **bytes, size_t *fsize);

void object_manager_register_object_type(char *class_name, char *bytecodes, int_t size);

char *resolveImportedClassName(char *class_name, map_t *imports_table);

#include "types/string.h"

char *class_path = 0;

/**
 * caller is responsible for freeing the pointer;
 * @param _self object pointer.
 * @return a string representation of the object.
 */
char *obj_to_string(void *_self) {
    char *buff = (char *) z_alloc_or_die(20);
    snprintf(buff, 20, "obj@%d", (int_t) _self);
    return buff;
}

static map_t *known_types_map = NULL;
static native_fnc_t *native_strlen_wrapper = NULL;
static native_fnc_t *native_keysize_wrapper = NULL;
static native_fnc_t *native_keylist_wrapper = NULL;

struct operations string_operations = {
        str_to_string
};
struct operations object_operations = {
        obj_to_string
};

/**
 * initialize the object manager with the specified class path.
 * @param cp
 */
void object_manager_init(char *cp) {
    class_path = cp;
    if (cp == NULL) {
        class_path = ".";
    }
    known_types_map = map_new(sizeof(z_type_info_t));
    native_strlen_wrapper = wrap_native_fnc(native_strlen);
    native_keysize_wrapper = wrap_native_fnc(native_object_key_size);
    native_keylist_wrapper = wrap_native_fnc(native_object_key_list);
}

/**
 * tries to retrieve a known type. if fails, loads it.
 *
 * @param class_name class -or type- name to load.
 * @return z_type_info_t.
 */
z_type_info_t *object_manager_get_or_load_type_info(char *class_name, map_t *imports_table) {
    z_type_info_t *type_info = (z_type_info_t *) map_get(known_types_map, class_name);
    if (type_info == NULL) {
        char *bytes = 0;
        size_t fsize;
        class_name = resolveImportedClassName(class_name, imports_table);
        load_class_code(class_name, &bytes, &fsize);
        object_manager_register_object_type(class_name, bytes, fsize);
        interpreter_run_static_constructor(bytes, class_name);
        type_info = (z_type_info_t *) map_get(known_types_map, class_name);
    }
    return type_info;
}

char *resolveImportedClassName(char *class_name, map_t *imports_table) {
    if (imports_table != NULL) {
            char** imported_path_ptr = (char**)map_get(imports_table, class_name);
            if (imported_path_ptr!= NULL){
                class_name = *imported_path_ptr;
            }
        }
    return class_name;
}

/**
 * register a type.
 * @param class_name name of this type.
 * @param bytecodes compiled byte codes.
 * @param size size of bytecodes.
 */
void object_manager_register_object_type(char *class_name, char *bytecodes, int_t size) {
    z_type_info_t *type_info = (z_type_info_t *) (z_alloc_or_die(sizeof(z_type_info_t)));
    type_info->class_name = class_name;
    type_info->bytecode_stream = bytecodes;
    type_info->bytecode_size = size;
    type_info->static_variables = map_new(sizeof(z_reg_t));
    type_info->imports_table = map_new(sizeof(char *));
    map_insert(known_types_map, class_name, type_info);
}

/**
 * create an object with the specified class name.
 *
 * if the class name is null, creates a json object. *
 * else loads the class -if not loaded yet- and creates an instance of it.
 *
 * @param class_name class name to load. can be null.
 * @return z_object.
 */
Z_INLINE z_object_t *object_new(char *class_name, map_t *imports_table) {
    z_object_t *obj = (z_object_t *) z_alloc_or_die(sizeof(z_object_t));
    obj->properties = map_new(sizeof(z_reg_t));
    obj->key_list_cache;
    obj->ref_count = 0;
    if (class_name) {
        class_name = resolveImportedClassName(class_name, imports_table);
        z_type_info_t *object_type_info = (z_type_info_t *) map_get(known_types_map, class_name);
        char *bytes = 0;
        size_t fsize;
        if (object_type_info == NULL) {
            //search on classpath and load
            load_class_code(class_name, &bytes, &fsize);
            object_manager_register_object_type(class_name, bytes, fsize);
            object_type_info = (z_type_info_t *) map_get(known_types_map, class_name);
        }
        obj->ordinary_object.type_info = object_type_info;
        z_interpreter_state_t *initial_state = (z_interpreter_state_t *) z_alloc_or_die(sizeof(z_interpreter_state_t));
        initial_state->fsize = obj->ordinary_object.type_info->bytecode_size;
        initial_state->byte_stream = obj->ordinary_object.type_info->bytecode_stream;
        initial_state->current_context = NULL;
        initial_state->instruction_pointer = NULL;
        initial_state->class_name = class_name;
        obj->ordinary_object.saved_state = z_interpreter_run(initial_state);
        return obj;
    }
    obj->operations = object_operations;
    z_reg_t temp;
    temp.type = TYPE_NATIVE_FUNC;
    temp.val = (int_t) native_keysize_wrapper;
    map_insert_non_enumerable(obj->properties, "size", &temp);
    temp.val = (int_t) native_keylist_wrapper;
    map_insert_non_enumerable(obj->properties, "keys", &temp);
    return obj;
}

/**
 * loads a class into known types map.
 *
 * first checks if a compiles .zcl file is exists. if so, loads.
 * if not, tries to find the source file and compile it.
 *
 * @param class_name class name to load.
 * @param bytes pointer to pointer of compiled bytes.
 * @param fsize pointer to file size.
 */
void load_class_code(const char *class_name, char **bytes, size_t *fsize) {
    size_t size = (int_t) strlen(class_name) + (int_t) strlen(class_path) + 5;
    char *file_to_load = (char *) z_alloc_or_die(size);
    snprintf(file_to_load, size, "%s/%s.zcl", class_path, class_name);
    FILE *f = fopen(file_to_load, "rb");
    if (f == NULL) {
#ifndef NO_DYNAMIC_COMPILATION
        //maybe not compiled yet?
        snprintf(file_to_load, size, "%s/%s.zs", class_path, class_name);
        f = fopen(file_to_load, "rb");
        if (f == NULL) {
            error_and_exit("cannot find class");
        } else {
            fclose(f);
            *bytes = compile_file(file_to_load, fsize);
        }
#else
        error_and_exit("cannot find class");
#endif
    } else {
        fseek(f, 0, SEEK_END);
        (*fsize) = (size_t) ftell(f);
        fseek(f, 0, SEEK_SET);
        (*bytes) = (char *) z_alloc_or_die((*fsize) + 1);
        fread((*bytes), (*fsize), 1, f);
        fclose(f);
    }
}
/**
 * create a function call context object.
 * @return z_object_t.
 */
Z_INLINE z_object_t *context_new() {
    z_object_t *obj = (z_object_t *) z_alloc_or_die(sizeof(z_object_t));
    obj->ref_count = 0;
    obj->context_object.symbol_table = NULL;
    obj->context_object.catches_list = NULL;
    return obj;
}

Z_INLINE z_object_t *function_ref_new(uint_t start_addr, void *parent_context, z_interpreter_state_t *state) {
    z_object_t *obj = (z_object_t *) z_alloc_or_die(sizeof(z_object_t));
    obj->ref_count = 0;
    obj->function_ref_object.start_address = start_addr;
    obj->function_ref_object.parent_context = parent_context;
    obj->function_ref_object.responsible_interpreter_state = state;
    obj->operations = object_operations;
    return obj;
}

Z_INLINE z_object_t *class_ref_new(char *name) {
    z_object_t *obj = (z_object_t *) z_alloc_or_die(sizeof(z_object_t));
    obj->ref_count = 0;
    obj->class_ref_object.value = name;
    obj->operations = object_operations;
    return obj;
}

Z_INLINE z_object_t *string_new(char *data) {
    z_object_t *obj = (z_object_t *) z_alloc_or_die(sizeof(z_object_t));
    obj->ref_count = 0;
    obj->string_object.value = data;
    obj->operations = string_operations;
    obj->properties = map_new(sizeof(z_reg_t));
    z_reg_t temp;
    temp.type = TYPE_NATIVE_FUNC;
    temp.val = (int_t) native_strlen_wrapper;
    map_insert_non_enumerable(obj->properties, "length", &temp);
    return obj;
}

map_t *build_symbol_table(const char *data) {
    map_t *symbol_table = map_new(sizeof(int_t));
    int_t pos = 0;
    uint_t size = *((uint_t *) (data + pos));
    pos += sizeof(int_t);
    for (int_t i = 0; i < size; i++) {
        int_t value = i + 1;
        char *item = (char *) (data + pos);
        pos += strlen(item) + 1;
        map_insert(symbol_table, item, &value);
    }
    return symbol_table;
}

#endif //ZEROSCRIPT_OBJECT_C
