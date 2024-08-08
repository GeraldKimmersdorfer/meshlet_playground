/*
 * Copyright (C) 2024, Gerald Kimmersdorfer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "../thirdparty/IconsFontAwesome5.h"

 /// <summary>
 /// Needs to be called after the setup of the dear gui and changes the color theme.
 /// WARNING: lightMode is buggy!
 /// </summary>
 /// <param name="darkMode">dark/light mode flag</param>
 /// <param name="alpha">The overall alpha level of the theme</param>
void activateImGuiStyle(bool darkMode = true, float alpha = 0.2F);

void setupImGuiFonts();
