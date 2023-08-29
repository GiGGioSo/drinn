package com.giggioso.drinn

import android.content.Context
import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.MutableTransitionState
import androidx.compose.animation.slideInHorizontally
import androidx.compose.animation.slideOutHorizontally
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.requiredHeight
import androidx.compose.foundation.layout.requiredWidth
import androidx.compose.foundation.layout.wrapContentSize
import androidx.compose.foundation.layout.wrapContentWidth
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.text.selection.LocalTextSelectionColors
import androidx.compose.foundation.text.selection.TextSelectionColors
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextFieldDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawWithContent
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalFocusManager
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.giggioso.drinn.ui.theme.DrinnTheme
import java.io.BufferedReader
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.io.InputStreamReader
import java.net.InetSocketAddress
import java.net.Socket
import java.net.SocketTimeoutException
import java.net.UnknownHostException
import kotlin.concurrent.thread
import kotlin.math.min


const val DRINN_DEFAULT_PORT: Int = 27333
const val DRINN_LAST_IP_FILENAME: String = "last_ip.txt"
const val DRINN_STX: Char = '#'
const val DRINN_ETX: Char = '~'
const val CD_ON_ERROR: Int = 3
const val CD_ON_SUCCESS: Int = 10

class MainActivity : ComponentActivity() {

    var lastIpAlreadyLoaded: Boolean = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            DrinnTheme {
                CreateMainUi()
            }
        }
    }

    @OptIn(ExperimentalMaterial3Api::class)
    @Preview
    @Composable
    private fun CreateMainUi() {
        var smallerSizePixel: Int = min(
            this.resources.displayMetrics.widthPixels,
            this.resources.displayMetrics.heightPixels
        )
        var inputIp by remember { mutableStateOf("") }
        if (!lastIpAlreadyLoaded) {
            try {
                inputIp = File(
                    applicationContext.filesDir,
                    DRINN_LAST_IP_FILENAME
                ).readText(Charsets.UTF_8)
                lastIpAlreadyLoaded = true
            } catch (e: Exception) {
            }
        }
        var feedbackMsg by remember { mutableStateOf("") }
        var feedbackColor by remember { mutableStateOf(Color.Green) }
        var countDownState = remember {
            MutableTransitionState(0).apply {
                targetState = 0
            }
        }
        var feedbackShowing by remember { mutableStateOf(false) }
        var drinnButtonEnabled by remember { mutableStateOf(true) }
        var density: Float = this.resources.displayMetrics.density
        var smallerSizeDp: Dp = Dp(0.9f * (smallerSizePixel / density))
        val focusRequester = remember { FocusRequester() }
        val focusManager = LocalFocusManager.current
        Surface(
            modifier = Modifier.fillMaxSize(),
            color = MaterialTheme.colorScheme.background
        ) {
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .clickable(
                        interactionSource = remember { MutableInteractionSource() },
                        indication = null,
                    ) { focusManager.clearFocus() },
            ) {}
            Column(
                modifier = Modifier.fillMaxHeight(),
                verticalArrangement = Arrangement.SpaceBetween
            ) {
                OutlinedTextField(
                    modifier =
                    Modifier
                        .fillMaxWidth()
                        .wrapContentWidth(align = Alignment.CenterHorizontally)
                        .requiredWidth(smallerSizeDp)
                        .padding(top = 32.dp)
                        .focusRequester(focusRequester),
                    value = inputIp,
                    onValueChange = { inputIp = it.trim() },
                    label = {
                        Text(
                            text = "IP",
                            color = Color.Black,
                            fontSize = 25.sp,
                            fontWeight = FontWeight.Bold
                        )
                    },
                    supportingText = {
                        Text(
                            modifier = Modifier
                                .fillMaxWidth()
                                .wrapContentWidth(Alignment.CenterHorizontally),
                            text = "DRINN server local IP",
                            fontSize = 18.sp,
                        )
                    },
                    singleLine = true,
                    textStyle = TextStyle(
                        color = Color.Black,
                        fontWeight = FontWeight.Bold,
                        fontSize = 30.sp
                    ),
                    shape = RoundedCornerShape(10),
                    keyboardOptions = KeyboardOptions(
                        autoCorrect = false,
                        keyboardType = KeyboardType.Number,
                        imeAction = ImeAction.Done,
                    ),
                    colors = TextFieldDefaults.textFieldColors(
                        containerColor = Color.Transparent,
                        textColor = Color.Black,
                        focusedSupportingTextColor = Color.Black,
                        unfocusedSupportingTextColor = Color.Transparent,
                        focusedIndicatorColor = Color.Black,
                        unfocusedIndicatorColor = Color.Black,
                        focusedLabelColor = Color.Black,
                        unfocusedLabelColor = Color.Black,
                        cursorColor = Color.Black,
                        selectionColors = TextSelectionColors(
                            handleColor = Color.Black,
                            backgroundColor = Color.Gray
                        ),
                    ),
                )
            }
            OutlinedButton(
                modifier = Modifier
                    .requiredWidth(smallerSizeDp)
                    .requiredHeight(smallerSizeDp),
                onClick = {
                    focusManager.clearFocus()
                    Log.d("IP USED", inputIp)
                    thread {
                        if (countDownState.targetState == countDownState.currentState) {
                            drinnButtonEnabled = false

                            try {
                                File(
                                    applicationContext.filesDir,
                                    DRINN_LAST_IP_FILENAME
                                ).writeText(inputIp, Charsets.UTF_8)
                            } catch (e: Exception) {
                            }

                            feedbackMsg = sendDrinn(inputIp)
                            feedbackShowing = true

                            if (feedbackMsg == "DRINN successful!") {
                                feedbackColor = Color.Green
                                countDownState.targetState = CD_ON_SUCCESS
                            } else {
                                feedbackColor = Color.Red
                                countDownState.targetState = CD_ON_ERROR
                            }
                            var secondsElapsed: Int = 0
                            while (countDownState.currentState < countDownState.targetState) {
                                Thread.sleep(1000L)
                                countDownState.targetState--
                                secondsElapsed++
                                if (secondsElapsed >= CD_ON_ERROR - 1) {
                                    feedbackShowing = false
                                }
                            }
                            drinnButtonEnabled = true
                        }
                    }
                },
                enabled = drinnButtonEnabled,
                colors = ButtonDefaults.buttonColors(
                    containerColor = Color.Transparent,
                    contentColor = Color.Black,
                    disabledContainerColor = Color.Transparent,
                    disabledContentColor = Color.Black,
                ),
                border = BorderStroke(
                    width = 15.dp,
                    brush = Brush.horizontalGradient(
                        when {
                            drinnButtonEnabled -> listOf(Color.Black, Color.Black)
                            else -> listOf(Color.Black, Color.Black)
                        }
                    )
                )
            ) {
                val startingFontSize = 80.sp
                var adaptedFontSize by remember { mutableStateOf(startingFontSize) }
                var readyToDraw by remember { mutableStateOf(false) }
                Text(
                    text = when {
                        (countDownState.isIdle) -> "DRINN"
                        else -> countDownState.targetState.toString()
                    },
                    fontSize = adaptedFontSize,
                    maxLines = 1,
                    softWrap = false,
                    modifier = Modifier.drawWithContent {
                        if (readyToDraw) {
                            drawContent()
                        }
                    },
                    onTextLayout = { textLayoutResult ->
                        if (textLayoutResult.didOverflowWidth) {
                            adaptedFontSize *= 0.9f
                        } else {
                            readyToDraw = true
                        }
                    }
                )
            }
            AnimatedVisibility(
                visible = feedbackShowing,
                enter = slideInHorizontally { fullWidth ->
                    -fullWidth
                },
                exit = slideOutHorizontally { fullWidth ->
                    fullWidth
                }
            ) {
                Text(
                    modifier = Modifier
                        .fillMaxSize()
                        .wrapContentSize(Alignment.BottomCenter)
                        .padding(bottom = 135.dp),
                    text = feedbackMsg,
                    fontSize = 24.sp,
                    color = feedbackColor
                )
            }
            Text(
                modifier = Modifier
                    .fillMaxSize()
                    .wrapContentSize(Alignment.BottomCenter)
                    .padding(bottom = 50.dp),
                text = "Made by GiGGioSo",
                fontSize = 14.sp,
                color = Color.Gray
            )
        }
    }

    private fun sendDrinn(ip: String, port: Int = DRINN_DEFAULT_PORT): String {
        var address: InetSocketAddress = InetSocketAddress(ip, port);
        val clientSocket: Socket = Socket()
        try {
            clientSocket.connect(address, 1000)
        } catch (e: UnknownHostException) {
            Log.e("Error", e.stackTraceToString())
            return "Unknown IP\nDRINN was unsuccessful"
        } catch (e: IOException) {
            // throw e
            Log.e("Error", e.stackTraceToString())
            return "Error opening socket\nDRINN was unsuccessful"
        } catch (e: IllegalArgumentException) {
            Log.e("Error", e.stackTraceToString())
            return "Invalid IP\nDRINN was unsuccessful"
        } catch (e: SecurityException) {
            Log.e("Error", e.stackTraceToString())
            return "Security error\nDRINN was unsuccessful"
        }

        clientSocket.soTimeout = 1000

        val outputStream = clientSocket.getOutputStream()
        val inputStream = BufferedReader(InputStreamReader(clientSocket.getInputStream()))

        val sendBuffer: ByteArray = (DRINN_STX + "DRINN" + DRINN_ETX).toByteArray()
        Log.d("DEBUG", "Sending: ${sendBuffer.toString()}")
        outputStream.write(sendBuffer)
        Log.d("DEBUG", "SENT!")

        var messageBuffer = CharArray(2048)

        val result: Int = try {
            inputStream.read(messageBuffer, 0, messageBuffer.size)
        } catch (e: IOException) {
            clientSocket.close()
            return "Error reading server response\nDRINN might have been unsuccessful"
        } catch (e: SocketTimeoutException) {
            clientSocket.close()
            return "Timed-out reading server response\nDRINN might have been unsuccessful"
        }

        Log.d("DEBUG", "RECEIVED!")

        if (messageBuffer[0] != DRINN_STX) {
            clientSocket.close()
            return "STX not found\nDRINN might have been unsuccessful"
        }

        val etxIndex = messageBuffer.indexOf(DRINN_ETX)
        if (etxIndex == -1) {
            clientSocket.close()
            return "ETX not found\nDRINN might have been unsuccessful"
        }

        val messageReceived: String = String(messageBuffer.copyOfRange(0, etxIndex + 1))

        if (messageReceived != DRINN_STX + "DRINN" + DRINN_ETX) {
            clientSocket.close()
            return "$messageReceived\nDRINN was unsuccessful"
        }

        clientSocket.close()
        return "DRINN successful!"
    }
}