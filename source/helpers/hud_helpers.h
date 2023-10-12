#pragma once

/// <summary>
/// Needs to be called after the setup of the dear gui and changes the color theme.
/// WARNING: lightMode is buggy!
/// </summary>
/// <param name="darkMode">dark/light mode flag</param>
/// <param name="alpha">The overall alpha level of the theme</param>
void activateImGuiStyle(bool darkMode = true, float alpha = 0.2F);

void StyleColorsSpectrum();