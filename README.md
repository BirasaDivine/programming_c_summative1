# Summative C Projects

This repository contains several C programming projects, each located in its own subdirectory. Each project demonstrates different aspects of C programming, including shell implementation, file management, function dispatching, web scraping, and embedded systems.

---

## Assessment Outcomes

Students will be able to:

- Manipulate strings using C language
- Use function pointers properly in C language
- Design circuits and deploy the right program
- Implement multiple threads
- Manipulate the file system using structures
- Write sound bash scripts

---

## Instructions

- The assessment project will be completed on an individual basis.
- The assessment will be graded out of 32 points.
- Each student is required to submit a video presenting their work (max 8 minutes, complete and comprehensive).
- No extensions will be granted after the due date.
- Keep your GitHub repo public until grades are published by the RO.

---

## Directory Structure

- `1_smart_home_system/` — Embedded smart home system simulation ([Tinkercad Project Link](https://www.tinkercad.com/things/3Qp2uu0rIht-project-1-smart-home-system?sharecode=slODaZz1SwD_IXdf38iAMwXUeDtKN9QfDhnUnaagfMM))
- `2_menu_shell/` — Mini shell implementation (`minishell`)
- `3_student_management_system/` — Student management system (`student_management`)
- `4_dispatcher/` — Dynamic function dispatcher for file operations (`dispatcher`)
- `5_scraper/` — Multithreaded web scraper using libcurl and pthreads (`scraper`)

---

## Project Details

### 1. Smart Home System

**Description:**
Develop an embedded system application that integrates sensors (e.g., temperature, motion), controllers, and actuators to simulate a smart home system. The system processes sensor input and triggers actions like turning on lights, sounding alarms, or controlling appliances.

**Key Features:**
- Use pointers and dynamic memory management to store sensor data
- Implement file handling to log sensor readings
- Use multi-threading to handle multiple tasks (e.g., data collection and response actions simultaneously)

**Demo & Circuit:**
[Tinkercad Project Link](https://www.tinkercad.com/things/3Qp2uu0rIht-project-1-smart-home-system?sharecode=slODaZz1SwD_IXdf38iAMwXUeDtKN9QfDhnUnaagfMM)

---

### 2. Mini Shell with Built-in Commands

**Description:**
Build a custom command-line shell that supports basic Unix commands, redirection, and piping.

**Key Features:**
- Implement basic command execution (e.g., `cat`, `ls`, `touch`, `pwd`, etc.)
- Add support for I/O redirection (`<`, `>`)
- Implement piping (`|`) between commands
- Support built-in commands like `exit`

---

### 3. Student Management System using Structures

**Description:**
A menu-driven system to add, view, search, update, and delete student records using structures. The system stores student details like name, ID, age, course, and grades.

**Key Features:**
- Use structures to store student details
- Implement file handling for persistent
 storage of records
- Include search and sorting algorithms for efficient retrieval
- Support basic CRUD operations (Create, Read, Update, Delete)

---

### 4. Dynamic Function Dispatcher

**Description:**
Implements a dynamic function dispatcher that selects and executes different file operations at runtime using function pointers and manages memory allocation efficiently.

**Key Features:**
- Use function pointers to dynamically select and execute functions (e.g., add data to file, count lines/chars, change case)
- Implement `malloc()` and `free()` for dynamic memory management
- Allow users to use function handlers dynamically

---

### 5. Multi-threaded Web Scraper

**Description:**
A multi-threaded web scraper that fetches data from multiple URLs in parallel using POSIX threads (pthreads). Each thread downloads HTML content and saves it to a separate
 file.

**Key Features:**
- Use pthreads to fetch multiple web pages in parallel
- Save downloaded content to separate text files (one file per thread)
- Implement basic error handling (e.g., if a URL is unreachable)

---

## Building All Projects

A single `Makefile` is provided in the `summative/` directory to build all projects at once.

### Prerequisites

- GCC (or compatible C compiler)
- `make`
- For the scraper: `libcurl` and `pthread` development libraries (e.g., `libcurl4-openssl-dev` on Debian/Ubuntu)

### Build Everything

From the `summative/` directory, run:

```sh
make
```

This will build all projects and place the binaries in their respective subdirectories.

### Clean All Binaries

To remove all compiled binaries:

```sh
make clean
```

---

## Building Individual Projects

You can also build each project separately:

- Mini Shell:
  ```sh
  make menu_shell
  ```
- Student Management System:
  ```sh
  make student_management
  ```
- Dispatcher:
  ```sh
  make dispatcher
  ```
- Scraper:
  ```sh
  make scraper
  ```

---

## Running the Programs

Navigate to the corresponding subdirectory and run the binary:

### 1. Mini Shell

```sh
cd 2_menu_shell
./minishell
```

A simple shell supporting basic commands, pipes, redirection, and background execution.

### 2. Student Management System

```sh
cd 3_student_management_system
./student_management
```

Menu-driven system to add, view, search, update, and delete student records (stored in `students.dat`).

### 3. Dispatcher

```sh
cd 4_dispatcher
./dispatcher
```

Menu-driven utility to perform various operations on `data.txt` (add text, count lines/chars, change case, etc.).

### 4. Scraper

```sh
cd 5_scraper
./scraper
```

Multithreaded web scraper that fetches five predefined URLs and saves their HTML content as `output_1.html`, `output_2.html`, etc.

---

## Notes

- The scraper requires internet access and the `libcurl` and `pthread` libraries.
- The student management system stores data in a binary file (`students.dat`) in its directory.
- All programs are written in standard C and should compile on any Unix-like system with the required libraries.

---

## License

This repository is for educational purposes.