# ATM Simulator

A console-based ATM (Automated Teller Machine) simulator written in **C**. It mimics the core workflow of a real ATM — authenticating a user, performing banking operations, and keeping records through persistent data files, transaction logs, and printable receipts.

> **Batch 27 · Team 08** — B.Tech CSE, UPES Dehradun
> Course: Programming in C

---

## Table of Contents

- [Features](#features)
- [Project Structure](#project-structure)
- [Getting Started](#getting-started)
- [Usage](#usage)
- [How It Works](#how-it-works)
- [Concepts Used](#concepts-used)
- [Sample Output](#sample-output)
- [Limitations & Future Scope](#limitations--future-scope)
- [Team](#team)

---

## Features

<!-- TODO: Edit this list so it matches exactly what your program does. Delete anything you did not implement. -->

- User authentication using account number and PIN
- Balance enquiry
- Cash deposit
- Cash withdrawal with balance validation
- Persistent storage of account data between runs (`data/`)
- Transaction logging with timestamps (`logs/`)
- Receipt generation for transactions (`Receipt/`)
- Input validation and error handling

---

## Project Structure

```
Batch27_Team08_ATMSimulator/
├── sourcecode.c    # Main program source code
├── data/           # Account data files (persistent storage)
├── logs/           # Transaction logs
├── Receipt/        # Generated transaction receipts
└── README.md       # Project documentation
```

| Path | Purpose |
|------|---------|
| `sourcecode.c` | Contains all program logic: menus, authentication, and transactions |
| `data/` | Stores account information so balances persist between sessions |
| `logs/` | Records a history of activity for auditing and review |
| `Receipt/` | Holds receipts generated after transactions |

---

## Getting Started

### Prerequisites

- A C compiler such as **GCC** or **Clang**
- A terminal (Linux, macOS, or Windows with MinGW / WSL)

Check your compiler:

```bash
gcc --version
```

### Clone the Repository

```bash
git clone https://github.com/21vergil/Batch27_Team08_ATMSimulator.git
cd Batch27_Team08_ATMSimulator
```

### Compile

```bash
gcc sourcecode.c -o atm
```

### Run

```bash
./atm          # Linux / macOS
atm.exe        # Windows
```

> **Note:** Run the program from the project's root folder so it can find the `data/`, `logs/`, and `Receipt/` directories.

---

## Usage

1. Launch the program.
2. Enter your **account number** and **PIN** when prompted.
3. Choose an option from the main menu:

```
1. Check Balance
2. Deposit
3. Withdraw
4. Exit
```

<!-- TODO: Update the menu above to match your actual menu. -->

4. After a transaction, a receipt is saved in `Receipt/` and the event is recorded in `logs/`.

### Demo Credentials

<!-- TODO: Add a test account so evaluators can try the program. -->

| Account Number | PIN |
|----------------|-----|
| _your test account_ | _your test PIN_ |

---

## How It Works

1. **Load data** – Account details are read from files in `data/`.
2. **Authenticate** – The user enters credentials, which are checked against stored records.
3. **Process transactions** – The chosen operation is validated (e.g. sufficient balance for withdrawals) and the balance is updated.
4. **Save state** – Updated data is written back to `data/` so changes persist.
5. **Log and receipt** – A timestamped entry is added to `logs/` and a receipt is written to `Receipt/`.

---

## Concepts Used

<!-- TODO: Keep only what your code genuinely uses. -->

- Functions and modular programming
- Structures (`struct`) for account records
- File handling (`fopen`, `fprintf`, `fscanf`, `fclose`)
- Control flow: loops, `switch`, and conditionals
- Input validation
- Time functions (`time.h`) for timestamps

---

## Sample Output

<!-- TODO: Paste a real run of your program here. -->

```
========== ATM SIMULATOR ==========
Enter Account Number: XXXXXXXX
Enter PIN: ****

Login successful!

1. Check Balance
2. Deposit
3. Withdraw
4. Exit
Enter choice:
```

---

## Limitations & Future Scope

- PINs are stored/handled in a simple way; a real system would use hashing and encryption.
- Single-user, console-only interface.
- Possible improvements:
  - PIN change and account lockout after failed attempts
  - Fund transfer between accounts
  - Mini-statement of recent transactions
  - Admin mode for creating and managing accounts
  - A graphical interface

---

## Team

**Team 08 — Batch 27**

| Name | SAP ID |
|------|--------|------|
| Chinmay | 590042217 
| Anvesh  | 590042455
| Naman   | 590033059

**University of Petroleum and Energy Studies (UPES), Dehradun**

---

## License

This project was developed for academic purposes.
