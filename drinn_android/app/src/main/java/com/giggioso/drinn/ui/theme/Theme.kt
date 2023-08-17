package com.giggioso.drinn.ui.theme

import android.app.Activity
import android.view.WindowManager
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.SideEffect
import androidx.compose.ui.platform.LocalView
import androidx.core.view.WindowCompat

/* Other default colors to override
background = Color(0xFFFFFBFE),
surface = Color(0xFFFFFBFE),
onPrimary = Color.White,
onSecondary = Color.White,
onTertiary = Color.White,
onBackground = Color(0xFF1C1B1F),
onSurface = Color(0xFF1C1B1F),
*/

private val drinnColorScheme = darkColorScheme(
    primary = DRINN_Yellow,
    secondary = PurpleGrey80,
    tertiary = Pink80,
    background = DRINN_Yellow
)

// private val DarkColorScheme = DRINN_ColorScheme

// private val LightColorScheme = DRINN_ColorScheme

@Composable
fun DrinnTheme(
    // darkTheme: Boolean = isSystemInDarkTheme(),
    // Dynamic color is available on Android 12+
    // dynamicColor: Boolean = true,
    content: @Composable () -> Unit
) {
    val view = LocalView.current
    if (!view.isInEditMode) {
        SideEffect {
            val window = (view.context as Activity).window
            WindowCompat.setDecorFitsSystemWindows(window, false)
            window.setFlags(
                WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
                WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS
            )
            // WindowCompat.getInsetsController(window, view).isAppearanceLightStatusBars = darkTheme
            // window.statusBarColor = colorScheme.primary.toArgb()
        }
    }

    MaterialTheme(
        colorScheme = drinnColorScheme,
        typography = Typography,
        content = content
    )
}