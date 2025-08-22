```
██████╗  █████╗ ██████╗ ████████╗    ██████╗  █████╗ ███████╗██╗  ██╗    ██████╗ ██████╗ ███████╗
██╔══██╗██╔══██╗██╔══██╗╚══██╔══╝    ██╔══██╗██╔══██╗██╔════╝██║  ██║    ╚════██╗██╔══██╗██╔════╝
██████╔╝███████║██████╔╝   ██║       ██████╔╝███████║███████╗███████║     █████╔╝██║  ██║███████╗
██╔══██╗██╔══██║██╔══██╗   ██║       ██╔══██╗██╔══██║╚════██║██╔══██║     ╚═══██╗██║  ██║╚════██║
██████╔╝██║  ██║██║  ██║   ██║       ██████╔╝██║  ██║███████║██║  ██║    ██████╔╝██████╔╝███████║
╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝       ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝    ╚═════╝ ╚═════╝ ╚══════╝
 ```

This project is my best attempt at recreating the hit game "Bart Bash" for the 3ds handheld console.
I spent my entire summer just programming this game which felt like the last stages of hell, so i hope it was worth the time, everything is programmed in C++, the assets are from the official Bart Bash website. (https://bartbash.com/)

Please dont read my code...

If you happen to find any bugs, please don't report because i'm too lazy to fix them.

How to play:
1. Open the .3dsx file from homebrew launcher (or cia if i happen to make a cia build later on)
2. Press start to begin your epic bart bash 3ds adventure
3. Select 1-6 barts with the stylus on the bottom screen
4. Press A to mark that you are ready to drop the bart
5. Select the position you want to drop the bart in with the stylus, and when you are ready, press A again
6. Profit

In the game you also have access to Copper and Gold paints which you can use on barts to boost their multiplier, you have one of each one when you begin the game,
but you also can purchase new ones with gems which you get from gem barts.

Please don't play this game

Build instructions:

To build the project you need the devkitpro toolchain, and also libopus and the other neccesities for it, if they arent already installed.
You should be able to just navigate to the projects root folder and run make, if you have issues, just follow whatever error you get.

PS: if the game crashes blame the low budget, not me.
