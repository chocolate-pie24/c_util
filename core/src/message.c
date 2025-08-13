/**
 * @file message.c
 * @author chocolate-pie24
 * @brief メッセージ標準出力機能実装
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <memory.h>
#include <stdio.h>
#include <stdarg.h>

#include "core/message.h"
#include "core/core_string.h"
#include "core/core_memory.h"

static CORE_STRING_ERROR_CODE msg_header_create(MESSAGE_SEVERITY severity_, core_string_t* const header_);

void message_output(MESSAGE_SEVERITY severity_, const char* const format_, ...) {
    FILE* out = (MESSAGE_SEVERITY_ERROR == severity_) ? stderr : stdout;

    core_string_t header = CORE_STRING_INITIALIZER;
    if(CORE_STRING_SUCCESS != msg_header_create(severity_, &header)) {
        fprintf(out, "message_output - Failed to create message header.\n");
        return;
    }

    core_string_t tail = CORE_STRING_INITIALIZER;
    if(CORE_STRING_SUCCESS != core_string_copy_from_char("\033[0m\n", &tail)) {
        fprintf(out, "message_output - Failed to create message tail.\n");
        return;
    }

    core_string_t message = CORE_STRING_INITIALIZER;
    if(CORE_STRING_SUCCESS != core_string_copy(&header, &message)) {
        fprintf(out, "message_output - Failed to copy message header.\n");
        return;
    }
    core_string_t body = CORE_STRING_INITIALIZER;
    if (CORE_STRING_SUCCESS != core_string_copy_from_char(format_, &body)) {
        fprintf(out, "message_output - Failed to copy message body.\n");
        return;
    }
    if(CORE_STRING_SUCCESS != core_string_concat(&body, &message)) {
        fprintf(out, "message_output - Failed to copy message format.\n");
        return;
    }
    if(CORE_STRING_SUCCESS != core_string_concat(&tail, &message)) {
        fprintf(out, "message_output - Failed to copy message tail.\n");
        return;
    }

    va_list args;
    va_start(args, format_);
    vprintf(core_string_cstr(&message), args);
    va_end(args);

    core_string_destroy(&header);
    core_string_destroy(&tail);
    core_string_destroy(&message);
}

/**
 * @brief メッセージの重要度種別に応じてメッセージ先頭に付加する文字列を生成する
 *
 * @param severity_ メッセージ重要度
 * @param header_ メッセージ先頭文字列格納先バッファ
 * @return true 生成成功
 * @return false 生成失敗
 */
static CORE_STRING_ERROR_CODE msg_header_create(MESSAGE_SEVERITY severity_, core_string_t* const header_) {
    FILE* out = (MESSAGE_SEVERITY_ERROR == severity_) ? stderr : stdout;
    CORE_STRING_ERROR_CODE ret = CORE_STRING_SUCCESS;
    switch(severity_) {
        case MESSAGE_SEVERITY_ERROR:
            ret = core_string_copy_from_char("\033[1;31m[ERROR] ", header_);
            break;
        case MESSAGE_SEVERITY_WARNING:
            ret = core_string_copy_from_char("\033[1;33m[WARNING] ", header_);
            break;
        case MESSAGE_SEVERITY_INFORMATION:
            ret = core_string_copy_from_char("\033[1;35m[INFORMATION] ", header_);
            break;
        case MESSAGE_SEVERITY_DEBUG:
            ret = core_string_copy_from_char("\033[1;34m[DEBUG] ", header_);
            break;
        default:
            fprintf(out, "msg_header_create - Undefined message severity.\n");
            ret = CORE_STRING_RUNTIME_ERROR;
    }
    return ret;
}
