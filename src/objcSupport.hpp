/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2025  mhahnFr
 *
 * This file is part of the LeakSanitizer.
 *
 * The LeakSanitizer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The LeakSanitizer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with the
 * LeakSanitizer, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef objcSupport_hpp
#define objcSupport_hpp

#ifdef __APPLE__
#include <objc/runtime.h>

extern "C" id objc_msgSend(id, SEL, ...);

#define CLASS(name) objc_getClass(#name)
#define SELECTOR(name) sel_registerName(#name)

#define CALL_ARGS(receiver, selName, ...) objc_msgSend((id) (receiver), SELECTOR(selName), __VA_ARGS__)
#define CALL(receiver, selName) objc_msgSend((id) (receiver), SELECTOR(selName))
#define CALL_CLASS_ARGS(clsName, selName, ...) CALL_ARGS(CLASS(clsName), selName, __VA_ARGS__)
#define CALL_CLASS(clsName, selName) CALL(CLASS(clsName), selName)

#ifdef OBJC_SUPPORT_EXTRA
# define _1(receiver, selName, ...) CALL_ARGS(receiver, selName, __VA_ARGS__)
# define _2(receiver, selName) CALL(receiver, selName)
# define _3(clsName, selName, ...) CALL_CLASS_ARGS(clsName, selName, __VA_ARGS__)
# define _4(clsName, selName) CALL_CLASS(clsName, selName)
#endif

#endif

#endif /* objcSupport_hpp */
