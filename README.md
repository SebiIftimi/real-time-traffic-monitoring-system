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

4. To monitor the number of vehicles inside the tunnel, this number is constantly checked.

5. Gas and smoke sensors are also continually monitored to prevent gas leaks and fires. If the gas/smoke sensors are triggered, the entrance to the tunnel will be blocked, and the exit will be unblocked (if it was previously locked). The entrance will be unblocked once gas or smoke is no longer detected inside the tunnel.

6. If the panic button is pressed, vehicles will be directed to exit the tunnel, and the entrance will be blocked.


# Aplication Architecture

The application is divided into several tasks corresponding to the different entities that interact and perform specific operations. In this implementation, tasks are akin to execution threads. The tasks within the application include:

1. Main Task:
   - Initialization: Sets up the semaphores used and creates threads for Entry, Exit, Monitoring, Smoke, Gas, and the Panic Button. After initialization, it waits for the user to enter commands via the keyboard: '1' to allow a vehicle into the tunnel, '2' to lock/unlock the entrance, and '3' to lock/unlock the exit.
    - Vehicle Entry (Command '1'): When '1' is entered via the keyboard, corresponding to a vehicle's entry into the tunnel, it checks the number of vehicles in the tunnel and whether the entrance is locked. If all checks are cleared, the vehicle will be allowed to enter.
    - Entrance Control (Command '2'): When '2' is entered, corresponding to locking/unlocking the tunnel entrance, access to the tunnel will be locked or unlocked accordingly. Vehicles will be able to enter the tunnel if the entrance is unlocked and all conditions for entry are met.
    - Exit Control (Command '3'): When '3' is entered, corresponding to locking/unlocking the exit, the tunnel exit will be locked or unlocked accordingly. The entrance to the tunnel will also be locked if the exit is locked.

2. Entry Task:
    - When an 'Entry' thread is executed, it checks the conditions for entering the tunnel and the current number of vehicles inside.
    - Vehicle Entry Conditions: If all the entry conditions are met, the vehicle is allowed to enter. Upon entry, the 'Entry' task notifies the 'Monitoring' task that a vehicle has entered the tunnel and subsequently alerts the 'Exit' task about the new vehicle's presence.
    - Blocking Entry: If any of the entry conditions are not satisfied, entry into the tunnel is blocked to prevent any new vehicles from entering.

3. Exit Task
    - This task runs continuously. When all conditions for exiting the tunnel are met, a vehicle leaves the tunnel. Upon exit, the 'Exit' task notifies the 'Monitoring' task that a vehicle has exited, and then informs the 'Entry' task, signaling that a new vehicle can enter. If any of the exit conditions are not met, the exit from the tunnel is blocked to ensure safety and compliance with traffic regulations.

4. Smoke Detection Task
    - This task runs continuously and generates a value that determines whether smoke is detected in the tunnel. If smoke is detected, the entrance is blocked and the exit is unlocked to facilitate safe evacuation. Once smoke is no longer detected, this information is relayed to the 'Entry' task.

5. Gas Detection Task
    - Similar to the smoke detection task, this task continuously monitors for the presence of gas in the tunnel. If gas is detected, the entrance is blocked and the exit is unlocked. Once no gas is detected, the 'Entry' task is informed so that normal tunnel operations can resume.
  
6. Panic Button Task

    - This task continuously monitors the state of the panic button. When the button is pressed, it signals an emergency by notifying the 'Entry' task to block the tunnel entrance and informing the 'Exit' task to unlock the exit. This ensures that vehicles inside the tunnel can evacuate quickly and safely in an emergency.













