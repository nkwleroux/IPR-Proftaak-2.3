#ifndef lcd_menu_H
#define lcd_menu_H

#include "i2c-lcd1602.h"

//init
#undef USE_STDIN

//I2C init
#define I2C_MASTER_NUM           I2C_NUM_0
#define I2C_MASTER_TX_BUF_LEN    0                     // disabled
#define I2C_MASTER_RX_BUF_LEN    0                     // disabled
#define I2C_MASTER_FREQ_HZ       100000
#define I2C_MASTER_SDA_IO        CONFIG_I2C_MASTER_SDA
#define I2C_MASTER_SCL_IO        CONFIG_I2C_MASTER_SCL

//LCD screen definitions
#define LCD_NUM_ROWS			 4
#define LCD_NUM_COLUMNS			 40
#define LCD_NUM_VIS_COLUMNS		 20

// Menu size settings
#define MAX_MENU_ITEMS 6
#define MAX_MENU_KEYS 3
#define MAX_LCD_LINES 4

// Possible key presses
#define MENU_KEY_OK 0
#define MENU_KEY_LEFT 1
#define MENU_KEY_RIGHT 2

// Menu screen IDs
#define MENU_MAIN_ID_0 0
#define MENU_MAIN_ID_1 1
#define MENU_MAIN_ID_2 2

//Date Time
#define MENU_DATE_TIME_ID_0 3

//Weather
#define MENU_WEATHER_ID_0 4

//Settings
#define MENU_SETTINGS_ID_0 5

//Date Time
#define MENU_DATE_TIME_SIZE 2

typedef struct {
    unsigned int id;
    unsigned int otherIds[MAX_MENU_KEYS];
    char *menuText[MAX_LCD_LINES];
    void (*fpOnKeyEvent[MAX_MENU_KEYS])(void);
    void (*fpOnMenuEntryEvent)(void);
    void (*fpOnMenuExitEvent)(void);
} menu_item_t;

typedef struct {
    i2c_lcd1602_info_t *lcd_info;
    menu_item_t *menuItems;
    unsigned int currentMenuItemId;
} menu_t;

/**
 * @brief  Menu Task which is used to start up the menu
 * @param  pvParameter: Parameter used for the xCreateTask method.
 */
void menu_task(void * pvParameter);

/**
 * @brief  Creates a menu pointer and initializes all required components.
 * @return menuPointer
 */
menu_t *menu_create_menu(void);

/**
 * @brief  Clears all data and free objects from memory.
 * @param  *menu: Object which holds the ids and events.
 */
void menu_free_menu(menu_t *menu);

/**
 * @brief  Displays default welcome message.
 * @param  *menu: Object which holds the ids and events.
 */
void menu_display_welcome_message(menu_t *menu);

/**
 * @brief  Display all menu items on LCD
 * @param  *menu: Object which holds the ids and events.
 * @param  menuItemId: ID for which menu the text will be retreived from.
 */
void menu_display_menu_item(menu_t *menu, int menuItemId);

/**
 * @brief  Displays menu scroll menu on lcd
 *         by writing currentMenuItem and the item before and after
 * @param  *menu: Object which holds the ids and events.
 */
void menu_display_scroll_menu(menu_t *menu);

/**
 * @brief  Handles key press by switching to new item or doing an onKeyEvent
 * @param  *menu: Object which holds the ids and events.
 * @param  key: Key id used to call the correct event.
 */
void menu_handle_key_event(menu_t *menu, int key);

/**
 * @brief  Navigate to given menu item
 * @param  menuItem: Index/id of the menu to navigate to.
 */
void menu_go_to_item(int menuItem);

#endif // lcd-menu