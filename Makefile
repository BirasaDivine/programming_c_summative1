# Makefile for building all C projects in summative subdirectories

.PHONY: all clean menu_shell student_management dispatcher scraper

CC = gcc
CFLAGS = -Wall -Wextra -O2

# Project directories and sources
MENU_SHELL_DIR = 2_menu_shell
MENU_SHELL_SRC = $(MENU_SHELL_DIR)/minishell.c
MENU_SHELL_BIN = $(MENU_SHELL_DIR)/minishell

STUDENT_MGMT_DIR = 3_student_management_system
STUDENT_MGMT_SRC = $(STUDENT_MGMT_DIR)/student_management.c
STUDENT_MGMT_BIN = $(STUDENT_MGMT_DIR)/student_management

DISPATCHER_DIR = 4_dispatcher
DISPATCHER_SRC = $(DISPATCHER_DIR)/dispatcher.c
DISPATCHER_BIN = $(DISPATCHER_DIR)/dispatcher

SCRAPER_DIR = 5_scraper
SCRAPER_SRC = $(SCRAPER_DIR)/scraper.c
SCRAPER_BIN = $(SCRAPER_DIR)/scraper

all: menu_shell student_management dispatcher scraper

menu_shell: $(MENU_SHELL_BIN)

$(MENU_SHELL_BIN): $(MENU_SHELL_SRC)
	$(CC) $(CFLAGS) -o $@ $<

student_management: $(STUDENT_MGMT_BIN)

$(STUDENT_MGMT_BIN): $(STUDENT_MGMT_SRC)
	$(CC) $(CFLAGS) -o $@ $<

dispatcher: $(DISPATCHER_BIN)

$(DISPATCHER_BIN): $(DISPATCHER_SRC)
	$(CC) $(CFLAGS) -o $@ $<

scraper: $(SCRAPER_BIN)

$(SCRAPER_BIN): $(SCRAPER_SRC)
	$(CC) $(CFLAGS) -o $@ $< -lcurl -lpthread

clean:
	rm -f $(MENU_SHELL_BIN) $(STUDENT_MGMT_BIN) $(DISPATCHER_BIN) $(SCRAPER_BIN)
