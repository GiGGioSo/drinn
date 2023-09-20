# drinn
A simple server-client application that plays a bell when a specific messagge is received through a TCP connection.

## Did this ever happen to you?

- You are listening to loud music on your headphones.
- Your mom is shouting at you from downstairs.
- She keeps shouting for 10 minutes.
- She comes upstairs. __Angry__.
- She pokes you and you realize you are in trouble.
- You are forced to keep an ear out in order to hear when she calls you.
- You are no longer able to enjoy music.

This is what would have happened if you had `drinn`:
- You are listening to loud music on your headphones.
- Your mom is shouting at you from downstairs.
- She notices you can't hear her and decides to use the `drinn` application on her phone.
- You hear the bell ringing in your headphones, you stop the music and respond to your mom.
- You get back to what you were listening. __Happy__.

## Server setup (Windows)

Steps:
- Obtain the server executable (`drinn.exe`) by either downloading it from the [release page](https://github.com/GiGGioSo/drinn/releases) or by building it yourself (use the `build.bat` file)
- Make sure that the files `drinn.exe` and `drinn.mp3` are in the same directory
- Execute `drinn.exe`:
    - With the flag `--set-volume <volume>` you can change the volume at which the drinn is played
    - With the flag `--no-adapters-info` you disable the message displayed at the start of the program that lists all the network adapters and the relative IPs
    - With the flag `--debug` you enable additional debug messages
    - With the flag `--set-autostart` makes it so the program automatically start at system start-up, always using the same flags that were used along side this one
    - With the flag `--help` displays a help message explaining how to use the various flags
- Now you can wait for DRINNs to arrive

### How to find the IP of my computer
It's important to know the IP of your computer in order to properly set up the Android application.
There are multiple ways of finding it.

__The easy one__:
- Execute `drinn.exe` and look at the list of network adapters that is printed.
- If you are using the Wi-Fi then look for the adapter called something like "Wi-Fi" _(the same goes for "Ethernet")_

__The difficult one__:
- Deal with it yourself

## Application setup (Android)

__WHEN INSTALLING THE APP YOUR PHONE WILL PROBABLY WARN YOU AND ADVICE YOU NOT TO INSTALL THE APPLICATION, BECAUSE IT'S NOT SIGNED, SO THE PHONE DOESN'T KNOW WHERE IT COMES FROM AND DOESN'T LIKE IT. THIS WILL BE FIXED SOONER OR LATER, PROBABLY WHEN I'LL TRY TO LOAD THE APP IN THE PLAY STORE__

Steps:
- Obtain the application installer (`drinn.apk`) by downloading it from the [release page](https://github.com/GiGGioSo/drinn/releases) and install it.
- Open the app, you will be presented with a big button in the center and an input field on the top
- In the input field on the top you will need to insert the IP address of the computer on which the server is running _(the one optained during the server setup)_
- Now you can press the button, and hopefully a DRINN will be received on the other end

## Tips and tricks

### Set static IP for the server
Per ora è un segreto :)

### DRINN from outside the local network
Per ora è un segreto :)

## To-Do
- Create an iOS application
