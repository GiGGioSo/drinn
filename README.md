# drinn
A simple server-client application that plays a bell when a specific messagge is received through a TCP connection.

## Did this ever happen to you
- You are listening to loud music on your headphones.
- Your mom is shouting at you from downstairs.
- She keeps shouting for 10 minutes.
- She comes upstairs. __Angry__.
- She pokes you and you realize you are in trouble.
- You are forced to keep an ear out in order to hear when she calls you.
- You are no longer able enjoy music.

This is what would have happened if you had `drinn`:
- You are listening to loud music on your headphones.
- Your mom is shouting at you from downstairs.
- She notices you can't hear her and decides to use the `drinn` application on her phone.
- You hear the bell ringing in your headphones, you stop the music and respond to your mom.
- You get back to what you were listening. __Happy__.

## Usage
__SERVER__:
- Obtain the server executable (`drinn.exe`) by either downloading it from the release page or by building it yourself (use the `build.bat`)
- Make sure that the files `drinn.exe` and `drinn.mp3` are in the same directory
- Execute `drinn.exe` _(you can set the volume by passing a value between 0 and 1 as argument, default is 1)_

---

__THE CLIENT PRESENT NOW IS JUST FOR TESTING PURPOSES, THE PROPER APPLICATIONS WILL BE DEVELOPED SHORTLY__

__TEST_CLIENT__:
- Obtain the server executable (`client.exe`) by either downloading it from the release page or by building it yourself (use the `build.bat`)
- Execute `client.exe` _(you can set the `ip_address` of the server by passing it as argument, default is `localhost`)_

## To-Do
- Create an Android application
- Create an iOS application
