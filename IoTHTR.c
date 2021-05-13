#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Login using scanf(), compare with a database (hardcoded string) to allow
// access or sleep for intervals If its an admin or operator you get different
// results

// If admin then you directly use getters to get values individually
// maybe make a log for the admin that gets appended to after every 5 seconds or
// so also quit with ctrl+c

// If operator just printf stuf every 5 seconds
// If operator, stuff is printed, you quit with ctrl-c

#define BUFSIZE 4096
#define WARNING_TYPES 6

#define BLACK "\033[0;30m"
#define BLUE "\x1b[34;1m"
#define RED "\x1B[31m"
#define ORG_BG "\033[48;2;255;165;0m"
#define RED_BG "\033[48;2;255;0;0m"
#define YEL_BG "\033[48;2;255;255;0m"
#define GRE_BG "\033[48;2;0;255;0m"
#define YEL "\x1B[33m"
#define DEFAULT "\x1b[0m"

char *op_username = "op_username";
char *op_password = "op_password";
char *ad_username = "ad_username";
char *ad_password = "ad_password";

int DATA_MODE = 1;  // 0 is normal mode, 1 is extreme mode

struct DOUBLE_PAIR {
    double first;
    double second;
};
struct DATA {
    int type;        // type of warning (based on use case number)
    double current;  // primary sensor value when sent
    char *units;     // yoinked from RANGE

    // following values only used for gate warning
    int state;         // state of gate. 0 for closed, 1 for open
    double time_left;  // time in seconds until gate will open for train
};

struct WARNING {
    char *color;
    time_t time;
    struct DATA data;
};

struct RANGE {
    // the type of the range, based on use case
    int type;
    // above max results in RED
    double max;
    // below max but above mid results in ORANGE
    double mid;
    // below mid but above min results in YELLOW
    double min;
    // no warning if below min
    char *units;
};

bool crossingOpen;
int timeUntilCrossingOpen;
double speed, gyroscope, rpm, distanceToCrossing, proximity, acceleration;
// struct DOUBLE_PAIR position;

struct RANGE speed_range, gyroscope_range, slippage_range, proximity_range;

int random_int_in_range(int low, int high) {
    return low + rand() % (high - low + 1);
}

void getCrossingState() {
    int randint = random_int_in_range(0, 10000);
    double random = (double)randint / 10000;
    bool boolval = true;
    // true means open
    if (DATA_MODE == 0) {  // Normal Mode (more likely to be open for train)
        if (random < 0.2) {
            boolval = true;
        } else {
            boolval = false;
        }
    } else {  // Danger Mode (more likely to be closed for train)
        if (random < 0.8) {
            boolval = true;
        } else {
            boolval = false;
        }
    }
    if (boolval == false) {
        randint = random_int_in_range(0, 100);
        timeUntilCrossingOpen = randint;
    } else {
        timeUntilCrossingOpen = 0;
    }
    crossingOpen = boolval;
}

void getSpeed() {
    //     speed_range.type = 1;
    //     speed_range.max = 90;
    //     speed_range.mid = 87;
    //     speed_range.min = 83;
    int randint = random_int_in_range(0, 10000);
    double random = (double)randint / 10000;
    double val = 0;
    if (DATA_MODE == 0) {  // Normal Mode (range between 80 and 85)
        val = 80 + (5 * random);
    } else {  // Danger Mode (range between 80 and 110)
        val = 80 + (30 * random);
    }
    speed = val;
}

void getAcceleration() {
    // Returns value between -5 and 5 normally
    int randint = random_int_in_range(-10000, 10000);
    double random = (double)randint / 10000;
    acceleration = (random * 5);
}

void getGyroscope() {
    //     gyroscope_range.type = 2;
    //     gyroscope_range.max = .4;
    //     gyroscope_range.mid = .1;
    //     gyroscope_range.min = 0;
    int randint = random_int_in_range(-10000, 10000);
    double random = (double)randint / 10000;
    double val = 0;
    if (DATA_MODE == 0) {  // Normal Mode (values -0.3 to 0.3)
        val = (0.3 * random);
    } else {  // Danger Mode (values -3.0 to 3.0)
        val = 3 * random;
    }
    gyroscope = val;
}

void getProximity() {
    //     proximity_range.type = 4;
    //     proximity_range.max = .5;
    //     proximity_range.mid = 1;
    //     proximity_range.min = 10;
    int randint = random_int_in_range(0, 10000);
    double random = (double)randint / 10000;
    // proximity = 0;
    if (DATA_MODE == 0) {  // Normal Mode (values 50 to 0.5)
        proximity = 0.5 + (45.5 * random);
    } else {  // Danger Mode (values 2 to 0)
        proximity = (2 * random);
    }
}

void getRPM() {
    // double random = rand() / (double)32767;
    // 200 / wheel circumference = 42.4628450106 (top rpm for speed)
    int randint = random_int_in_range(-10000, 10000);
    double random = (double)randint / 10000;
    if (DATA_MODE == 0) {  // Normal Mode (values up to .3 difference from
                           // expected based on speed)
        rpm = speed / 4.71 + random * (.3);
    } else {  // Danger Mode (values up to 3 difference from expected based on
              // speed)
        rpm = speed / 4.71 + (3 * random);
    }
}

void getDistanceToCrossing() {
    // double random = rand() / (double)32767;
    int randint = random_int_in_range(0, 10000);
    double random = (double)randint / 10000;
    double proximity = 0;
    if (DATA_MODE == 0) {  // Normal Mode (values 0.10 to 5)
        proximity = 0.10 + (49.9 * random);
    } else {  // Danger Mode (values 2 to 0)
        proximity = (2 * random);
    }
    distanceToCrossing = proximity;
}

// void getCrossingState(){
//     int randint = (rand() % 10001) / 10000.0;
//     crossingOpen = (randint == 0);
// }

double calculateSlippage(double rpm, double speed) {
    //     0.75 meter wheel radius
    //     4.71 meter wheel circumference
    //     returns a calculation, not a random number
    getSpeed();
    getRPM();
    double wheel_speed = rpm * 4.71;
    return speed - wheel_speed;
}

struct WARNING getSpeedWarning() {
    struct WARNING w;
    struct DATA d;
    char *color;
    time_t t = time(NULL);
    getSpeed();
    if (speed >= speed_range.max) {
        color = RED_BG;
    } else if (speed >= speed_range.mid) {
        color = ORG_BG;
    } else if (speed >= speed_range.min) {
        color = YEL_BG;
    } else {
        color = DEFAULT;
    }

    d.type = speed_range.type;
    d.current = speed;
    d.units = speed_range.units;

    // d = {speed_range.type, speed};
    // w = {color, t, d};
    w.color = color;
    w.time = t;
    w.data = d;
    return w;
}

struct WARNING getSlippageWarning() {
    struct WARNING w;
    struct DATA d;
    char *color;
    time_t t = time(NULL);

    double slippage = calculateSlippage(rpm, speed);  // TODO implement

    if (abs(slippage) >= slippage_range.max) {
        color = RED_BG;
    } else if (abs(slippage) >= slippage_range.mid) {
        color = ORG_BG;
    } else if (abs(slippage) >= slippage_range.min) {
        color = YEL_BG;
    } else {
        color = DEFAULT;
    }

    // d = {slippage_range.type, slippage};
    d.type = slippage_range.type;
    d.current = slippage;
    d.units = slippage_range.units;
    w.color = color;
    w.time = t;
    w.data = d;
    return w;
}

struct WARNING getAngleWarning() {
    struct WARNING w;
    struct DATA d;
    char *color;
    time_t t = time(NULL);

    getGyroscope();
    if (abs(gyroscope) >= gyroscope_range.max) {
        color = RED_BG;
    } else if (abs(gyroscope) >= gyroscope_range.mid) {
        color = ORG_BG;
    } else if (abs(gyroscope) >= gyroscope_range.min) {
        color = YEL_BG;
    } else {
        color = DEFAULT;
    }

    // d = {gyroscope_range.type, gyroscope};
    d.type = gyroscope_range.type;
    d.current = gyroscope;
    d.units = gyroscope_range.units;
    w.color = color;
    w.time = t;
    w.data = d;
    return w;
}

struct WARNING getObstacleWarning() {
    struct WARNING w;
    struct DATA d;
    char *color;
    time_t t = time(NULL);

    getProximity();
    if (proximity <= proximity_range.min) {
        color = RED_BG;
    } else if (proximity <= proximity_range.mid) {
        color = ORG_BG;
    } else if (proximity <= proximity_range.max) {
        color = YEL_BG;
    } else {
        color = DEFAULT;
    }

    // d = {proximity_range.type, proximity};
    d.type = proximity_range.type;
    d.current = proximity;
    d.units = proximity_range.units;
    w.color = color;
    w.time = t;
    w.data = d;
    return w;
}

struct WARNING getCrossingWarning() {
    /*    For the severity to be determined as low (YELLOW)
    Gate state is open OR
    Gate state is closed but the gate will open when the train reaches the gate
    (determined by speed and time sent from gate) For the severity to be
    determined as medium (ORANGE) Gate state is closed AND Deceleration of train
    is sufficient to reach yellow state before reaching the gate (keep current
    deceleration) For the severity to be determined as high (RED) Gate state is
    closed AND Deceleration of train is insufficient to reach yellow state
    before reaching the gate (must increase deceleration)*/

    struct WARNING w;
    struct DATA d;
    char *color;
    time_t t = time(NULL);
    getCrossingState();
    getDistanceToCrossing();
    getAcceleration();
    double time_to_gate_s = speed * distanceToCrossing;
    double time_to_gate_a =
        (-speed * sqrt(speed * speed + 4 * distanceToCrossing * acceleration)) /
        (distanceToCrossing * (-2));

    if (crossingOpen || (timeUntilCrossingOpen < time_to_gate_s)) {
        color = YEL_BG;
    } else if (!crossingOpen && timeUntilCrossingOpen < time_to_gate_a) {
        color = ORG_BG;
    } else if (!crossingOpen && timeUntilCrossingOpen > time_to_gate_a) {
        color = RED_BG;
    } else {
        color = DEFAULT;
    }

    if (distanceToCrossing >= 40) {
        color = DEFAULT;
    }

    d.type = 5;
    d.current = distanceToCrossing;
    d.units = "km";
    w.color = color;
    w.time = t;
    w.data = d;
    return w;
}

struct WARNING getHornAlert() {
    struct WARNING w;
    struct DATA d;
    char *color;
    time_t t = time(NULL);

    if (distanceToCrossing <= 1.60934) {  // one mile in kilometers
        color = GRE_BG;
    } else {
        color = DEFAULT;
    }

    d.type = 6;
    d.current = distanceToCrossing;
    w.color = color;
    w.time = t;
    w.data = d;
    return w;
}

struct WARNING *isSafe() {
    struct WARNING *warnings = malloc(sizeof(struct WARNING) * WARNING_TYPES);
    warnings[0] = getCrossingWarning();
    warnings[1] = getAngleWarning();
    warnings[2] = getSlippageWarning();
    warnings[3] = getSpeedWarning();
    warnings[4] = getObstacleWarning();
    warnings[5] = getHornAlert();
    return warnings;
}

// slippage = 1
// gyro = 2
// speed = 3
// objontrack = 4
// getcrossing = 5
void TSNR(struct WARNING warning) {
    char *text1;
    char *text2;
    char *text3;
    
    switch (warning.data.type) {
        case 1:
            text1 = "Slippage Detected";
            if (warning.data.current < 0) {  // maybe flip
                text2 = "SUGGEST DECREASE SPEED";
            } else {
                text2 = "SUGGEST INCREASE SPEED";
            }
            break;
        case 2:
            text1 = "Dangerous Angle Detected";
            if (warning.data.current < 0) {
                text2 = "DOWNHILL";
            } else {
                text2 = "UPHILL";
            }
            break;
        case 3:
            text1 = "Dangerous Speed Detected";
            text2 = "TOO FAST; SLOW DOWN";
            break;
        case 4:
            text1 = "Object On Track Ahead";
            text2 = "APPLY BRAKES";
            break;
        case 5:
            text1 = "Gate Crossing Ahead";
            if (warning.data.state == 0) {
                text2 = "GATE CLOSED";
            } else {
                text2 = "GATE OPEN";
            }
            break;
        case 6:
            text1 = "Gate Crossing Ahead";
            if (warning.data.current <= 1.60934 &&
                warning.data.current >=
                    .015) {  // if one mile away but not yet reached
                text2 = "Blow Horn for 15 seconds";
            }
            if (warning.data.current <= .015) {  // reached
                text2 = "Blow Horn for 5 seconds";
            }
            break;
    }

    if (strcmp(warning.color, GRE_BG) == 0){
        text3 = "GREEN";
    }
    if (strcmp(warning.color, RED_BG) == 0){
        text3 = "RED";
    }
    if (strcmp(warning.color, YEL_BG) == 0){
        text3 = "YELLOW";
    }
    if (strcmp(warning.color, ORG_BG) == 0){
        text3 = "ORANGE";
    }
    

    printf("%s", BLACK);
    printf("%s", warning.color);
    printf("\n");
    printf("%s", text1);
    printf("\n");
    // printf("\n%s", warning.color);
    printf("%s", text2);
    printf("\n");
    // printf("\n%s", warning.color);
    printf("Current value: %f %s", warning.data.current, warning.data.units);
    printf("\n%s\n", DEFAULT);
    FILE *fp = fopen("logs.txt", "a");
    fputs(text3, fp);
    fputs(": ", fp);
    fputs(text1, fp);
    fputs("\n\t", fp);
    fputs(text2, fp);

    fputs("\n", fp);
    fclose(fp);
}

void logTrain() {
    FILE *fp = fopen("logs.txt", "a");
    fputs("--Raw Sensor Data for Above Warnings--", fp);
    fputs("\n", fp);

    time_t current_raw_time = time(0);
    fputs("Current time: ", fp);
    fputs(ctime(&current_raw_time), fp);
    // fputs("\n", fp);

    fputs("Current speed: ", fp);
    fprintf(fp, "%0.3lf", speed);
    fputs("\n", fp);

    fputs("Current gyroscope: ", fp);
    fprintf(fp, "%0.3lf", gyroscope);
    fputs("\n", fp);

    fputs("Current rpm: ", fp);
    fprintf(fp, "%0.3lf", rpm);
    fputs("\n", fp);

    fputs("Current distance to crossing: ", fp);
    fprintf(fp, "%0.3lf", distanceToCrossing);
    fputs("\n", fp);

    fputs("Gate is open?: ", fp);
    fputs(crossingOpen ? "true" : "false", fp);
    fputs("\n", fp);

    fputs("Time until crossing open: ", fp);
    fprintf(fp, "%i", timeUntilCrossingOpen);
    fputs("\n", fp);

    fputs("Current proximity to object: ", fp);
    fprintf(fp, "%0.3lf", proximity);
    fputs("\n", fp);

    fputs("Current acceleration: ", fp);
    fprintf(fp, "%0.3lf", acceleration);
    fputs("\n", fp);

    fputs("---------------------------------------\n\n", fp);

    fclose(fp);
}

void runTrain() {
    // normal running of the program for an operator.
    // every 5 seconds the screen updates with the top 5 most dangerous current
    // warnings

    struct WARNING *warnings;
    // printf("test");
    // speed = 100;
    // warnings = isSafe();
    int warnings_displayed;

    while (1) {
        warnings = isSafe();
        warnings_displayed = 0;
        printf("--------------------------------\n");
        for (int i = 0; i < WARNING_TYPES; i++) {
            if (strcmp(warnings[i].color, GRE_BG) == 0) {  // maybe do strcmp
                TSNR(warnings[i]);
                warnings_displayed++;
                if (warnings_displayed >= WARNING_TYPES) {
                    break;
                }
            }
        }
        for (int i = 0; i < WARNING_TYPES; i++) {
            if (strcmp(warnings[i].color, RED_BG) == 0) {  // maybe do strcmp
                TSNR(warnings[i]);
                warnings_displayed++;
                if (warnings_displayed >= WARNING_TYPES) {
                    break;
                }
            }
        }
        for (int i = 0; i < WARNING_TYPES; i++) {
            if (strcmp(warnings[i].color, ORG_BG) == 0) {  // maybe do strcmp
                TSNR(warnings[i]);
                warnings_displayed++;
                if (warnings_displayed >= WARNING_TYPES) {
                    break;
                }
            }
        }
        for (int i = 0; i < WARNING_TYPES; i++) {
            if (strcmp(warnings[i].color, YEL_BG) == 0) {  // maybe do strcmp
                TSNR(warnings[i]);
                warnings_displayed++;
                if (warnings_displayed >= WARNING_TYPES) {
                    break;
                }
            }
        }
        if (warnings_displayed != 0) {
            // if at least one warning was displayed for the current timestep,
            // log the state of the train and the time
            logTrain();
        } else {
            // no warnings displayed
            printf("No warnings displayed\n");
        }
        sleep(5);
    }
}

int main() {
    srand(time(NULL));

    speed_range.type = 3;
    speed_range.max = 90;
    speed_range.mid = 87;
    speed_range.min = 83;
    speed_range.units = "km/h";

    gyroscope_range.type = 2;
    gyroscope_range.max = 1.5;
    gyroscope_range.mid = 1;
    gyroscope_range.min = .4;
    gyroscope_range.units = "degrees";  // grade?

    slippage_range.type = 1;
    slippage_range.max = 1;
    slippage_range.mid = .5;
    slippage_range.min = .1;
    slippage_range.units = "km/h difference";

    proximity_range.type = 4;
    proximity_range.max = 5;
    proximity_range.mid = 1;
    proximity_range.min = .5;
    proximity_range.units = "km";
LOGIN:
    printf(" ");
    char *username_input;
    if ((username_input = (char *)malloc(sizeof(char) * BUFSIZE)) == NULL) {
        fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }
    char *password_input;
    if ((password_input = (char *)malloc(sizeof(char) * BUFSIZE)) == NULL) {
        fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // Potential requirements: Limited login attempts, lock user out after
    // exceeding maximum number of attempts
    int attempts = 0;
    bool isAdmin = false;

    printf("IoT LOGIN (Use CTRL+C to exit)\n\n");
    while (true) {
        // uncomment to short circut the login process
        // break;
        if (attempts == 3) {
            printf("The police have been notified.\n");
            for (int i = 10; i > 0; i--) {
                printf("TIMED OUT: %d minutes remaining\n", i);
                sleep(60);
            }
            printf("\n");
            attempts = 0;
        }

        printf("Enter username: ");
        scanf("%s", username_input);
        printf("Enter password: ");
        scanf("%s", password_input);

        if (strcmp(username_input, op_username) ==
            0) {  // if username matches operator
            if (strcmp(password_input, op_password) !=
                0) {  // if password does not match operator password
                printf("Invalid password received.\n\n");
                attempts++;
                continue;
            } else {
                break;  // password matches operator password
            }
        } else if (strcmp(username_input, ad_username) ==
                   0) {  // if username matches admin
            if (strcmp(password_input, ad_password) !=
                0) {  // if password does not match admin
                printf("Invalid password received.\n\n");
                attempts++;
                continue;
            } else {
                isAdmin = true;  // system knows that the password matches admin
                                 // password and that the user is the admin
                break;
            }
        } else {
            printf(
                "Invalid username received.\n\n");  // Username matches neither
                                                    // admin or operator
            attempts++;
            continue;
        }
    }

    free(username_input);
    free(password_input);
    attempts = 0;

    if (isAdmin) {
        char *input;
        struct RANGE *rangeptr;
        bool cancel = false;

        if ((input = (char *)malloc(sizeof(char) * BUFSIZE)) == NULL) {
            fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
            return EXIT_FAILURE;
        }

        while (true) {
            double temp_min;
            double temp_mid;
            double temp_max;

            printf(
                "\nInput '~' to toggle simulation mode.\nInput 'l' to view "
                "logs.\nInput 'r' to delete logs.\nInput '%d' to access "
                "slippage detection.\nInput '%d' to "
                "access angle detection.\nInput '%d' to access speed "
                "detection.\nInput '%d' to access proximity detection.\nInput "
                "'-1' to log out.\n",
                slippage_range.type, gyroscope_range.type, speed_range.type,
                proximity_range.type);

            char slippage_type_string[BUFSIZE];
            sprintf(slippage_type_string, "%d", slippage_range.type);
            char gyroscope_type_string[BUFSIZE];
            sprintf(gyroscope_type_string, "%d", gyroscope_range.type);
            char speed_type_string[BUFSIZE];
            sprintf(speed_type_string, "%d", speed_range.type);
            char proximity_type_string[BUFSIZE];
            sprintf(proximity_type_string, "%d", proximity_range.type);

            scanf("%s", input);

            if (strcmp(input, slippage_type_string) == 0) {
                printf("\nSlippage detection warning thresholds:\n");
                rangeptr = &slippage_range;
            } else if (strcmp(input, gyroscope_type_string) == 0) {
                printf("\nGyroscope detection warning thresholds:\n");
                rangeptr = &gyroscope_range;
            } else if (strcmp(input, speed_type_string) == 0) {
                printf("\nSpeed detection warning thresholds:\n");
                rangeptr = &speed_range;
            } else if (strcmp(input, proximity_type_string) == 0) {
                printf("\nProximity detection warning thresholds:\n");
                rangeptr = &proximity_range;
            } else if (strcmp(input, "-1") == 0) {
                printf("\nLogging out...\n");
                isAdmin = false;
                goto LOGIN;
            } else if (strcmp(input, "~") == 0) {
                printf("Toggling simulation mode.\n");
                DATA_MODE = !DATA_MODE;
                continue;
            } else if (strcmp(input, "l") == 0) {
                char c;
                FILE *fptr = fopen("logs.txt", "r");
                if (fptr == NULL) {
                    printf("No logs on record.\n");
                    continue;
                }
                c = fgetc(fptr);
                while (c != EOF) {
                    printf("%c", c);
                    c = fgetc(fptr);
                }

                fclose(fptr);
                continue;

            } else if (strcmp(input, "r") == 0) {
                remove("logs.txt");
                continue;
            } else {
                printf("Input '%s' not recognized.\n", input);
                continue;
            }

            if (rangeptr->type == proximity_range.type) {
                printf("Yellow: %f %s\nOrange: %f %s\nRed: %f %s\n",
                       rangeptr->max, rangeptr->units, rangeptr->mid,
                       rangeptr->units, rangeptr->min, rangeptr->units);
            } else {
                printf("Yellow: %f %s\nOrange: %f %s\nRed: %f %s\n",
                       rangeptr->min, rangeptr->units, rangeptr->mid,
                       rangeptr->units, rangeptr->max, rangeptr->units);
            }

            while (true) {
                if (rangeptr->type == proximity_range.type) {
                    printf(
                        "\nInput new max value (yellow warning threshold) or "
                        "'-1' to cancel. ");
                } else {
                    printf(
                        "\nInput new min value (yellow warning threshold) or "
                        "'-1' to cancel. ");
                }
                scanf("%s", input);
                char *endptr;
                double num = strtod(input, &endptr);
                if (*endptr) {
                    printf("Invalid double '%s' received.\n", input);
                } else if (num < 0) {
                    if (num == -1) {
                        cancel = true;
                        break;
                    } else {
                        printf("Threshold cannot be negative.\n");
                    }
                } else {
                    if (rangeptr->type == proximity_range.type) {
                        temp_max = num;
                    } else {
                        temp_min = num;
                    }
                    break;
                }
            }
            if (cancel) {
                cancel = false;
                continue;
            }
            while (true) {
                printf(
                    "\nInput new mid value (orange warning threshold) or '-1' "
                    "to cancel. ");
                scanf("%s", input);
                char *endptr;
                double num = strtod(input, &endptr);
                if (*endptr) {
                    printf("Invalid double '%s' received.\n", input);
                } else if (num < 0) {
                    if (num == -1) {
                        cancel = true;
                        break;
                    } else {
                        printf("Threshold cannot be negative.\n");
                    }
                } else {
                    if (rangeptr->type == proximity_range.type) {
                        if (num >= temp_max) {
                            printf(
                                "Mid value (%f %s) must be less than max value "
                                "(%f %s).\n",
                                num, rangeptr->units, temp_max,
                                rangeptr->units);
                        } else {
                            temp_mid = num;
                            break;
                        }
                    } else {
                        if (num <= temp_min) {
                            printf(
                                "Mid value (%f %s) must exceed min value (%f "
                                "%s).\n",
                                num, rangeptr->units, temp_min,
                                rangeptr->units);
                        } else {
                            temp_mid = num;
                            break;
                        }
                    }
                }
            }
            if (cancel) {
                cancel = false;
                continue;
            }
            while (true) {
                if (rangeptr->type == proximity_range.type) {
                    printf(
                        "\nInput new min value (red warning threshold) or '-1' "
                        "to cancel. ");
                } else {
                    printf(
                        "\nInput new max value (red warning threshold) or '-1' "
                        "to cancel. ");
                }
                scanf("%s", input);
                char *endptr;
                double num = strtod(input, &endptr);
                if (*endptr) {
                    printf("Invalid double '%s' received.\n", input);
                } else if (num < 0) {
                    if (num == -1) {
                        cancel = true;
                        break;
                    } else {
                        printf("Threshold cannot be negative.\n");
                    }
                } else {
                    if (rangeptr->type == proximity_range.type) {
                        if (num >= temp_max) {
                            printf(
                                "Min value (%f %s) must be less than max value "
                                "(%f %s).\n",
                                num, rangeptr->units, temp_max,
                                rangeptr->units);
                        } else if (num >= temp_mid) {
                            printf(
                                "Min value (%f %s) must be less than mid value "
                                "(%f %s).\n",
                                num, rangeptr->units, temp_mid,
                                rangeptr->units);
                        } else {
                            temp_min = num;
                            break;
                        }
                    } else {
                        if (num <= temp_min) {
                            printf(
                                "Max value (%f %s) must exceed min value (%f "
                                "%s).\n",
                                num, rangeptr->units, temp_min,
                                rangeptr->units);
                        } else if (num <= temp_mid) {
                            printf(
                                "Max value (%f %s) must exceed mid value (%f "
                                "%s).\n",
                                num, rangeptr->units, temp_mid,
                                rangeptr->units);
                        } else {
                            temp_max = num;
                            break;
                        }
                    }
                }
            }
            if (cancel) {
                cancel = false;
                continue;
            }
            rangeptr->min = temp_min;
            rangeptr->mid = temp_mid;
            rangeptr->max = temp_max;

            if (rangeptr->type == proximity_range.type) {
                printf(
                    "\nWarning thresholds updated.\nYellow: %f %s\nOrange: %f "
                    "%s\nRed: %f %s\n",
                    rangeptr->max, rangeptr->units, rangeptr->mid,
                    rangeptr->units, rangeptr->min, rangeptr->units);
            } else {
                printf(
                    "\nWarning thresholds updated.\nYellow: %f %s\nOrange: %f "
                    "%s\nRed: %f %s\n",
                    rangeptr->min, rangeptr->units, rangeptr->mid,
                    rangeptr->units, rangeptr->max, rangeptr->units);
            }
        }
    } else {
        // open IoT display with random sensor data producing warnings.
        runTrain();
    }
    return EXIT_SUCCESS;
}
