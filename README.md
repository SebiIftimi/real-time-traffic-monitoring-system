This project is designed to develop and simulate a real-time system dedicated to traffic monitoring within a tunnel. Its
primary goal is to optimize traffic flow and enhance safety for all participants.

The simulation was conducted on both the Arduino Mega 2560 board, using the FreeRTOS library, and the Debian Xenomai Linux
platform, utilizing the C/POSIX API. The selection of these environments was motivated by their provision of all essential 
mechanisms for synchronization and communication between tasks, including generalized semaphores, mutexes, timers, and conditional 
variables.

# Problem Overview 

1. For this project, we will assume that the tunnel has a single direction and one lane of traffic. For safety reasons, no more than five vehicles can be present in the tunnel at any given time.

2. The tunnel is equipped with entry and exit sensors, natural gas leak detectors, smoke detectors, and a panic button. Both the entrance and exit of the tunnel can be locked or unlocked by the tunnel operator.

3. Conditions for the proper operation of the tunnel:
    - When the number of vehicles inside the tunnel reaches five, the entrance to the tunnel will be blocked. 
    - If the gas/smoke sensors detect any gas or smoke inside the tunnel, or if the panic button is activated, the tunnel entrance will be immediately blocked.
    - If the tunnel operator blocks the exit, the entrance will also be blocked.

4. If the panic button is pressed, vehicles will be directed to exit the tunnel, and the entrance will be blocked.


# Aplication Architecture

 In this implementation, tasks are akin to execution threads. The tasks within the application include:

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


# Defining the Solution for Implementation

   # 1. Linux-Xenomai/ POSIX

The solution has been implemented on the Debian Xenomai Linux platform, utilizing the C/POSIX API. This environment was chosen for its comprehensive set of synchronization and communication mechanisms between tasks, such as generalized semaphores, mutexes, timers, and conditional variables. These tools are crucial for handling the complexities of real-time system operation within the tunnel simulation.

In this program, threads are used to simulate tunnel operations instead of processes, as threads allow easier sharing of resources, simpler and more efficient communication, and faster execution speeds.

To ensure the correct functioning of the application and to address as many possible operational scenarios of the tunnel as possible, the following mechanisms have been employed:

- **Semaphore `semafor_entry`** with an initial value of 5, to track the number of vehicles that can enter the tunnel. Any attempt by a vehicle to enter the tunnel is blocked by this semaphore if the maximum number of vehicles is reached.
  
- **Semaphore `semafor_exit`** with an initial value of 0, used by 'Entry' to notify 'Exit' that a vehicle has entered the tunnel. Initializing this semaphore with 0 ensures that vehicles cannot exit the tunnel before entering.
  
- **Semaphore `semafor_monitoring`** with an initial value of 0, used by 'Entry' and 'Exit' to notify 'Monitoring' when a vehicle enters or exits the tunnel.
  
- **Mutex `mutex_nr_m`** locked/unlocked by: 'Entry', when checking entry conditions to increment the `nr_cars` variable; 'Exit', when checking exit conditions to decrement the `nr_cars` variable; 'Monitoring', when verifying the number of vehicles in the tunnel to allow or block entry depending on whether the maximum number is reached; 'Panic Button', when the panic button is pressed and vehicles need to exit the tunnel.
  
- **Mutex `mutex_safty`** locked/unlocked by: 'Smoke' and 'Gas' when detecting smoke or gas in the tunnel to modify the `safety` variable, and 'Entry', when checking entry conditions.
  
- **Mutex `mutex_block_entry`** locked/unlocked by: 'Panic Button', when the panic button is pressed to modify the `block_entry` variable, and by 'Entry' when reading this variable to check the entry conditions.
  
- **Mutex `mutex_block_exit`** locked/unlocked by: 'Panic Button', when the panic button is pressed to modify the `block_exit` variable, and by 'Exit' when reading this variable to check the exit conditions.
  
- **Conditional variables `condvar_entry` and `condvar_exit`** used to restrict the operation of 'Entry' and 'Exit' tasks when the panic button is pressed. These are used in conjunction with `mutex_block_entry` and `mutex_block_exit`.
  
- **A timer** that triggers the panic button; when the issue signaled by the panic button is resolved, the timer is reset.

This implementation ensures a robust and responsive system capable of handling normal operations as well as emergency scenarios within the tunnel environment.

# 2. Arduino/ FreeRTOS.h

**The solution was implemented using the Arduino Mega 2560 board.**

**Libraries Used for Implementation:**

- **Arduino_FreeRTOS.h**: The primary library for FreeRTOS on Arduino, enabling the simultaneous execution of multiple tasks (threads), and task creation.
- **semphr.h**: Provides functionalities for manipulating semaphores and mutexes.
- **Wire.h**: Used for I2C communication.
- **LiquidCrystal_I2C.h**: Used to communicate with and control the LCD screen.

**Mechanisms Chosen to Ensure Correct Application Functioning:**

- **Semaphore** `sem_entry` and `sem_exit`: These ensure that there are no conflicts or race conditions when multiple tasks attempt - to modify the state of the entrance and exit.
- **Semaphore** `sem_monitoring`: Used to synchronize the monitoring task with the entry and exit tasks from the tunnel. After a vehicle enters or exits, this semaphore can be signaled to update the display on the LCD with the current number of vehicles.
- **Semaphore** `sem_turnOnLED`: Used to synchronize the operation of lighting the LEDs with the entry or exit of a vehicle from the tunnel.
- **Mutex** `mutex_panic`: Used for accessing the `Panic` variable.

**Components Used for Implementation:**

- **5 green LEDs**: Each LED lights up when a vehicle enters the tunnel and turns off when it exits.
- **1 red LED**: Indicates that the maximum number of vehicles has been reached in the tunnel or that the tunnel entrance has been blocked by the operator.
- **1 yellow LED**: Associated with the panic button to indicate activation.
- **Panic Button**: Used to signal emergencies.
- **200 ohm resistors**: Used to ensure proper current flow to the LEDs.
- 1602 IIC/I2C LCD screen: Displays the current number of vehicles in the tunnel.

# **Task Organization Chart for Linu-Xenomai/ POSIX**
![patr-6](https://github.com/SebiIftimi/real-time-traffic-monitoring-system/assets/104670681/28a1974c-203c-4036-a114-c525f687be21)

# **Task Organization Chart for Arduino**

![organigrama_arduino](https://github.com/SebiIftimi/real-time-traffic-monitoring-system/assets/104670681/59375446-084a-41e9-b674-f6087aeb17ad)


