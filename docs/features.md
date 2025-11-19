# SimpliOS Features

This document describes the user-facing features of SimpliOS, including the interactive shell, built-in commands, and applications.

## 1. Interactive Shell

The SimpliOS shell (`simplios>`) is the primary interface for interacting with the system. It supports standard command-line operations and basic line editing.

### 1.1 Line Editing
- **Typing**: Standard alphanumeric characters are echoed to the screen.
- **Backspace**: Deletes the character to the left of the cursor.
- **Left/Right Arrows**: Moves the cursor within the current line.
- **Up/Down Arrows**: Navigates through the command history (last 16 commands).
- **Enter**: Executes the current command.

### 1.2 Colors
The shell uses a color-coded output system:
- **Cyan**: Command prompt.
- **Green**: Success messages and information.
- **Yellow**: Warnings.
- **Red**: Error messages.

You can customize the default text color using the `setcolor` command.

## 2. Command Reference

### System Commands
- **`help`**: Displays a list of all available commands with brief descriptions.
- **`clear`**: Clears the terminal screen and resets the cursor to the top-left.
- **`uptime`**: Shows how long the system has been running (in ticks and seconds).
- **`status`**: Displays system status, including:
  - Ramdisk usage (files and bytes).
  - Scheduler information (tick count, active processes).
  - Current process details.
- **`ps`**: Lists all active processes with their PID, Name, State, and CPU usage.
- **`setcolor <fg> [bg]`**: Sets the default text color.
  - Arguments: Color names (e.g., `white`, `green`, `blue`, `red`).
  - Example: `setcolor green` sets the text to green.

### File Management
- **`ls`**: Lists all files currently stored in the ramdisk.
- **`create <filename>`**: Creates a new empty file.
  - Example: `create notes.txt`
- **`delete <filename>`**: Deletes a file from the ramdisk.
  - Example: `delete notes.txt`
- **`write <filename> <text...>`**: Overwrites a file with the provided text.
  - Example: `write notes.txt Hello World`
- **`cat <filename>`** (or `read`): Displays the contents of a file.
  - Example: `cat notes.txt`
- **`echo <text...>`**: Prints the provided text back to the console. Useful for testing.

### Tools & Games
- **`calc <num1> <op> <num2>`**: A simple integer calculator.
  - Supported operators: `+`, `-`, `*`, `/`.
  - Example: `calc 10 * 5` outputs `50`.
  - Note: Division by zero is handled safely.
- **`breakout`**: Launches the Breakout arcade game.

## 3. Breakout Game

SimpliOS includes a fully functional port of the classic Breakout game.

### 3.1 How to Play
- **Start**: Type `breakout` in the shell and press Enter.
- **Controls**:
  - **Left Arrow**: Move paddle left.
  - **Right Arrow**: Move paddle right.
  - **Space**: Launch the ball (at start or after losing a life).
  - **Q**: Quit the game and return to the shell.

### 3.2 Game Mechanics
- **Objective**: Destroy all bricks by deflecting the ball with your paddle.
- **Lives**: You start with 3 lives. If the ball hits the bottom of the screen, you lose a life.
- **Scoring**: Each brick destroyed awards 10 points.
- **Physics**: The ball bounces off walls, the paddle, and bricks.

### 3.3 Technical Details
The game runs in a custom loop hooked into the kernel's idle task. This allows it to run at a smooth 60 FPS (controlled by the PIT) without blocking interrupts.

## 4. Calculator

The `calc` command provides basic arithmetic capabilities directly in the shell.

### Usage
```text
simplios> calc 100 + 50
Result: 150
simplios> calc 20 / 4
Result: 5
```

### Error Handling
- **Division by Zero**: Prints a red error message and does not crash.
- **Invalid Operator**: Warns if an unsupported operator is used.
