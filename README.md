This project is designed to develop and simulate a real-time system dedicated to traffic monitoring within a tunnel. Its
primary goal is to optimize traffic flow and enhance safety for all participants.

Here's an academically toned English translation for your GitHub README:

The simulation was conducted on both the Arduino Mega 2560 board, using the FreeRTOS library, and the Debian Xenomai Linux
platform, utilizing the C/POSIX API. The selection of these environments was motivated by their provision of all essential 
mechanisms for synchronization and communication between tasks, including generalized semaphores, mutexes, timers, and conditional 
variables.

# Problem Overview 

1. For this project, we will assume that the tunnel has a single direction and one lane of traffic. For safety reasons, no more than five vehicles can be present in the tunnel at any given time.

2. The tunnel is equipped with entry and exit sensors, natural gas leak detectors, smoke detectors, and a panic button. Both the entrance and exit of the tunnel can be locked or unlocked by the tunnel operator.

3. Conditions for the proper operation of the tunnel:
    - When the number of vehicles inside the tunnel reaches five, the entrance to the tunnel will be blocked. Entry will be permitted again once the number of vehicles inside the tunnel drops below five.
    - If the gas/smoke sensors detect any gas or smoke inside the tunnel, or if the panic button is activated, the tunnel entrance will be immediately blocked.
    - If the tunnel operator blocks the exit, the entrance will also be blocked.
