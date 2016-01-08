/*
ADVENTURE

Author: Josh Seifert
Filename: seiferjo.adventure.c
Date Created / Last Modified: 04/24/2015 - 05/02/2015
Description: This program is a rudimentary implementation of a text-
	based adventure game, similar to its namesake game first created in
	the 1970's. The player starts in a randomly selected room, and navigates
	between rooms until they reach the predetermined ending room, upon which
	the game reports the path they travelled to reach the end. This program
	is an introduction to writing programs in C on UNIX systems, and basic
	file I/O. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>		//Seed for randomization
#include <dirent.h>		//For navigating file directories
#include <sys/types.h>	//Used to get process id
#include <sys/stat.h>	//Needed for mkdir command
#include <unistd.h>		//Tests for read/write permission

//Used when creating room files. Not used for gameplay purposes
struct room{
	int id;
	char *name;
	char *roomType;
	int adjacentRooms[6];
	int counter;
};

char *createRoomDirectory();
void createRooms(char *roomDir);
void playGame(char *roomDir);

int main()
{
	//Initializes pseudo-random number generator with unique seed
	srand(time(NULL));

	//Creates the folder to hold the room files
	char *roomDir = createRoomDirectory();

	//Creates the rooms, and writes them to the new folder
	createRooms(roomDir);

	//Starts the game interface
	playGame(roomDir);

	//When gameplay completes, exit succesfully.
	exit(0);
}

/*
Function: createRoomDirectory
Receives: None
Returns: roomDir, a char array containing the name of a newly created directory.
Description: Uses the author's username (seiferjo) and current UNIX process ID
	to create a unique directory where the game will write and access rooms files
	used in gameplay.
*/
char *createRoomDirectory()
{
	//Array used to hold name of room directory
	char *roomDir = malloc(100);

	/*Concatenate directory to a single string. getpid returns current process id,
	ensures unique folder name. Using sprintf versus a technique like strcat
	allows the int pid to be added to the roomDir string without converting
	data types*/
	sprintf(roomDir, "seiferjo.rooms.%d", getpid());

	//Make the directory, with full access for the user.
	int madeDir = mkdir(roomDir, 0770);
	if(madeDir < 0)
	{
		printf("Error: Unable to create directory.");
		exit(1);
	}

	//Minor formatting to help when referencing files in the directory later.
	strcat(roomDir, "/");
	return roomDir;
}

/*
Function: createRooms
Receives: roomDir, char array containing directory path
Returns: None
Description: Creates 7 files in roomDir directory, each representing a unique
	room in the cave. Files follow strict formatting rules, where each room has
	a unique name, 3 - 6 connections to other rooms, all connections are 
	reciprocal, and one room is randomly selected as the starting room, and another
	is randomly selected as the ending room.
*/

void createRooms(char *roomDir)
{
	//Rooms are initialized as an array of structs, which are then written to files.
	struct room *roomArray = malloc(7 * sizeof(struct room));

	//The names of the rooms. All names taken from Terry Pratchett's DiscWorld novel series.
	char *roomNames[10] = { "Ankh-Morpork", "Pseudopolis", "Klatch", "Uberwald",
		"Ramtops", "The Rim", "Ephebe", "Great Nef", "Krull", "Borogravia" };

	//Corresponds to roomName array, value changes to 0 when name is taken.
	int roomNameAvailable[10] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
	
	//For loop controllers
	int i, j, k;	

	//Counter holds the number of connections between each room.
	for (i = 0; i < 7; i++)
	{
		roomArray[i].counter = 0;
	}

	for (i = 0; i < 7; i++)					
	{
		//Internally, rooms are referenced by integer ID, easier than char arrays.
		roomArray[i].id = i;

		//Random number 0 - 9 corresponding to an index in the room name array.
		int roomNameNum = rand() % 10;
		
		//Room names may only appear once. Reroll if name taken.
		while (roomNameAvailable[roomNameNum] == 0)
		{
			roomNameNum = rand() % 10;
		}

		//If room name is available, assigns that name to the room
		roomArray[i].name = roomNames[roomNameNum];

		//Mark room name as taken, so it may not be reused.
		roomNameAvailable[roomNameNum] = 0;		

		//All rooms initialized as "Middle Rooms"
		roomArray[i].roomType = "MID_ROOM";				
	}
	
	/*Creates the connections between rooms. Loop through every room 3 times.
	Generates a random (non self-referential) numbered room to link to, and
	adds that link to both start and finish rooms. Checks to make sure link is
	unique before adding it. There must be a connection between the start and 
	ending rooms. This is not explicitly checked, as it is mathematically 
	guaranteed that an array of 7 rooms where each has at least 3 connections 
	is a completely connected graph.*/

	//Every room links to at least 3 rooms.
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 7; j++)
		{
			//The room number to which the current room connects.
			int connection;

			//Boolean value, to test if connection already exists.
			int unique = 1;
			do
			{
				unique = 1;
				
				//The ID of the room the current room attempts to connect to.
				connection = rand() % 7;

				//Compares the connection with the connections the room already has.
				for (k = 0; k < roomArray[j].counter; k++)
				{
					//If connection already exists, set bool flag to false
					if (connection == roomArray[j].adjacentRooms[k])
						unique = 0;
				}

				/*Generate new value if:
				1. The connection is the current room's ID (It connects to itself)
				2. The connection is one of the current room's first 2 connections.
				Note: The number is allowed to be a repeat, so long as the room already 
				has 3 total connections. This is so not every room has 5-6 connections*/
			} while (connection == roomArray[j].id || connection == roomArray[j].adjacentRooms[0] || connection == roomArray[j].adjacentRooms[1]);

			//Only add room connection if it is unique, does not already exist.
			if (unique == 1)			
			{ 
				roomArray[j].adjacentRooms[roomArray[j].counter] = connection;
				roomArray[j].counter++;

				//The room being linked to also links back to the current room.
				roomArray[connection].adjacentRooms[roomArray[connection].counter] = j;
				roomArray[connection].counter++;
			}
		}
	}

	//The number rooms that are set as the starting and ending points.
	int startRoom = rand() % 7;
	int endRoom = rand() % 7;

	//Ending room must be different room number than starting room.
	while (endRoom == startRoom)
	{
		endRoom = rand() % 7;
	}
	
	roomArray[startRoom].roomType = "START_ROOM";
	roomArray[endRoom].roomType = "END_ROOM";

	//Holds the string name of the room file
	char *roomFile = malloc(100);

	for (i = 0; i < 7; i++)
	{
		//File name is the room directory concatenated with the room name itself.
		strcpy(roomFile, roomDir);
		strcat(roomFile, roomArray[i].name);
		
		//Creates a file 
		FILE *oneRoom;
		if((oneRoom = fopen(roomFile, "w")) == NULL)
		{
			printf("Error: Unable to create room file.");
			exit(1);
		}

		//Writes all the details to the room files. Start with room name
		fprintf(oneRoom, "ROOM NAME: %s\n", roomArray[i].name);

		//"Counter" used here to write variable number of room connections
		for (j = 0; j < roomArray[i].counter; j++)
		{
			fprintf(oneRoom, "CONNECTION %d: %s\n", j + 1, roomArray[roomArray[i].adjacentRooms[j]].name);
		}
		
		//Last thing to write is the type of room. START, MID, or END
		fprintf(oneRoom, "ROOM TYPE: %s\n\n", roomArray[i].roomType);

		fclose(oneRoom);
	}

	/*All rooms files are now written. While it would be a lot easier to
	continue using the array, sadly, that is not the assignment. Goodbye, array!*/
	free(roomArray);
}

/*
Function: playGame
Receives: roomDir, char array containing directory path
Returns: None
Description: The main game loop of the program. Opens the room file
	directory, and finds the starting room. Tells user their current
	location, and adjacent rooms to which they may travel. Keeps track
	of the number of steps they take, and the rooms they traverse to reach
	their destination. Only allows traversal between rooms if the user
	enters in the exact name of the destination room. Ends when the
	user reaches the designated end room.
*/

void playGame(char *roomDir)
{
	//Number of steps taken to reach the end
	int numSteps = 0;
	//The rooms travelled to reach the end
	char path[30][20];
	//The file name of the current room
	char fileName[100];
	//The current room itself
	FILE *currentRoom;
	//For loop counter
	int i;
	//Stores the current room's number of connections
	int numConnections;
	//For player to enter desired destination.
	char whereTo[100];
	//Boolean value, checks if player entered valid room destination.
	int validDestination;

	//Details of the current room. Read in from files as player moves between rooms.
	char roomName[20];
	char roomConnections[6][20];
	char roomType[20];

	
	/*This following section opens the folder with the room files, and finds
	the starting room by searching every file for the line "START_ROOM."
	Information on opening the directory and cycling through files from
	http://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
	and information on searching each file for a specific line of text from
	http://www.codingunit.com/c-tutorial-searching-for-strings-in-a-text-file*/

	//The room directory to be opened.
	DIR *d;
	//Contains info about directory entries, filenames, types
	struct dirent *dir;
	//String that temporarily stores line of text as read in from file
	char line[100];

	//Opens the directory with the room files
	d = opendir(roomDir);
	if (d)
	{
		//Reads every file in the directory
		while ((dir = readdir(d)) != NULL)
		{
			//Only look at regular files (the room files, in this case)
			if (dir->d_type == DT_REG)
			{
				//fileName is the directory + name of file itself, from dirent struct
				strcpy(fileName, roomDir);
				strcat(fileName, dir->d_name);

				if ((currentRoom = fopen(fileName, "r")) == NULL)
				{
					printf("Error: Unable to open room file.");
					return;
				}

				//Reads through entire current file, 1 line at a time
				while (fgets(line, 100, currentRoom) != NULL)
				{
					//If line contains string "START_ROOM, it is the starting room
					if ((strstr(line, "START_ROOM")) != NULL)
					{
						//Save starting room name to begin main game loop
						strcpy(roomName, dir->d_name);
					}
				}
			}
		}

		/*The loop when player navigates between rooms. Runs indefinitely, until player
		reaches the end room.*/
		while (1)
		{
			strcpy(fileName, roomDir);
			strcat(fileName, roomName);
	
			//Open the current room room
			if ((currentRoom = fopen(fileName, "r")) == NULL)
			{
				printf("Error: Unable to open file");
				return;
			}
			
			numConnections = 0;
			
			//Reads in information from room file, one line at a time.
			while (fgets(line, 100, currentRoom) != NULL)
			{	
				//Lob off these newline characters for formatting purposes
				if (line[strlen(line) - 1] == '\n')
					line[strlen(line) - 1] = 0;
				
				//If line contains room name, copy it to roomName array
				if ((strstr(line, "ROOM NAME")))
					//Name starts at 11 characters from the start of the line
					strcpy(roomName, &line[11]);
				
				//If line contains room connection, copy it to roomConnections array
				if ((strstr(line, "CONNECTION")))
				{
					//Connection starts at 14 characters from the start of the line
					strcpy(roomConnections[numConnections], &line[14]);
					//roomConnections is a 2d array, increment the string array index.
					numConnections++;
				}
				
				//If line contains room type, copy it to roomType array
				if ((strstr(line, "ROOM TYPE")))
					//Type starts at 11 characters from the start of the line
					strcpy(roomType, &line[11]);
			}
			fclose(currentRoom);
	
			//Once the player enters a new room, add it to the list of visited rooms.
			strcpy(path[numSteps], roomName);
			//Increment the rooms visited counter.
			numSteps++;

			//Win condition, exits game loop. Compares room type to "END_ROOM"
			if (strcmp(roomType, "END_ROOM") == 0)
			{
				//numSteps - 1, because we do not count entering the start room as a step.
				printf("\nYOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\nYOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", numSteps - 1);
				
				//Print every room traversed, excluding start room.
				for (i = 1; i < numSteps; i++)
				{
					printf("%s\n",path[i]);
				}
				//This is the only way to break out of the game loop.
				return;
			}

			//If user did not win, tells them current room info, prompts for new location.
			do{
				validDestination = 0;

				printf("\nCURRENT LOCATION: %s\n", roomName);
				printf("POSSIBLE CONNECTIONS:");
				
				//Print the names of every adjacent room.
				for (i = 0; i < numConnections - 1; i++)
				{
					printf(" %s,", roomConnections[i]);
				}
				
				//Last adjacent room has slightly different formatting requirements.
				printf(" %s.\n", roomConnections[numConnections - 1]);
				printf("WHERE TO? >");
				
				//Reads user input into "whereTo".
				fgets(whereTo, 100, stdin);

				//Get rid of that pesky newline character on input.
				if (whereTo[strlen(whereTo) - 1] == '\n')
					whereTo[strlen(whereTo) - 1] = 0;
				
				//Checks if user's input equals any of the current room's connections.
				for (i = 0; i < numConnections; i++)
					
					//If there is a match, let user proceed with that room choice
					if (strcmp(whereTo, roomConnections[i]) == 0)
						validDestination = 1;
					
				//If no match, prompt user to try again.
				if (validDestination == 0)
					printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
				
			//Loop runs indefinitely, until user enters valid choice.
			} while (validDestination == 0);

			//If player enters a valid room name, restart loop with that room file
			strcpy(roomName, whereTo);
		}
	}
}