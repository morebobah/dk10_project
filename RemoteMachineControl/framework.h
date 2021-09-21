// header.h: включаемый файл для стандартных системных включаемых файлов
// или включаемые файлы для конкретного проекта
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Исключите редко используемые компоненты из заголовков Windows
// Файлы заголовков Windows
#include <atlstr.h>
#include <strsafe.h>
#include <wil/result.h>
#include <wil/com.h>
#include <windows.h>
#include <wrl.h>
// Файлы заголовков среды выполнения C
#include <cpprest/json.h>
#include <malloc.h>
#include <memory.h>
#include <memory>
#include <stdlib.h>
#include <tchar.h>
#include <map>

// Файлы заголовков специальных включений
#include "resource.h"
