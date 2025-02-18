#include <message.h>
#include <functions.h>

// function removes nodes from the linked list
Node* removeNode(Node** aircraftList, int id) {
    Node* current = *aircraftList;
    Node* previous = NULL;
    Node* node_to_remove = NULL;

    while (current != NULL) {
        if (current->data.id == id) {
            node_to_remove = current;
            if (previous != NULL) {
                previous->next = current->next;
            } else {
                *aircraftList = current->next;  // Update the list head when removing the first node
            }
            break; // Exit the loop, we found the node to remove
        }
        previous = current;
        current = current->next;
    }

    if (node_to_remove != NULL) {
        free(node_to_remove);
    }

    return current;
}


// Function to find a node with a specific ADS-B ID in the linked list
Node* findNode(Node** aircraftList, int id) {
    // returns true false statement
    bool exists = false;
    Node* current = *aircraftList;
    while (current != NULL) {
        if (current->data.id == id) 
        {
            exists = true;
            break;
        }
        // moves to next node in list
        current = current->next;
    }
    // case that there is no current node
    if(exists == false)
    {
       return NULL;
    }
    else
    {
        return current;
    }
}

// function which appends/ creates new nodes
void appendNodes(Node** aircraftList, ADSBPacket data, int* numPlanes) {    
    // allocated new node to the linked list memory
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    newNode->data = data;
    newNode->next = *aircraftList;
    *aircraftList = newNode;
    (*numPlanes)++;
}

//function which impliments a booleen to check where the aircraft's position is
bool EstimatePosition(Node** aircraftList, ADSBPacket updatedData, bool printer) {
    // defines a plane to not be in the aircraft's range to be true
    bool NotInRange = true;

    Node* updateNode = findNode(aircraftList, updatedData.id);
    // mathematics equations that calculate the current northa nd east positions by refering to elements in the linked list.
    if(updateNode != NULL) {
        double currentNorth = updateNode->data.north * 1000;
        double currentEast = updateNode->data.east * 1000;

        double heading = updateNode->data.heading * M_PI/180;
        //Vn
        
        double velocityNorth = updateNode->data.speed * sin(heading);
        
        double velocityEast = updateNode->data.speed * cos(heading);
        
        double newPacketTime = updatedData.time;
        double lastPacketTime = updateNode->data.time;

        // presets the estimated north and east positions to be 0 unless math is applied.
        double estimatedNorth = 0;
        double estimatedEast = 0;

        // calculation of the estimated north and east coordinates 
        estimatedNorth = (currentNorth + velocityNorth * (newPacketTime - lastPacketTime))/1000;
        estimatedEast = (currentEast + velocityEast * (newPacketTime - lastPacketTime))/1000;

        //checking if the aircraft is within the airspace range
        if ((pow(estimatedNorth,2) + pow(estimatedEast,2)) <= pow(OPERATION_ZONE,2)){
            // if the aircraft is within the range of the airspace it will return an estimated position based off calculations
            if (printer == true){
                printf("Aircraft (ID:%d): Estimated Position: N:%.1lf,E:%.1lf\n", updatedData.id, estimatedNorth, estimatedEast);
            }
            // redefining not in range to be false
            NotInRange = false;
        }
        else{
            // in the case that the aircraft is not in the area of operation it prints this statment. 
            if(printer == true){
                printf("Aircraft (ID:%d) not currently in area of operation\n", updatedData.id);
            }
            // removes this plane id from the linked list when it is out of the airspace
            removeNode(aircraftList, updatedData.id);
    
        }

    }
    else {
        printf("aircraft with ID %d not found :( /n)", updatedData. id);
    }
    return NotInRange;
}

// function scans the input from each line in the text file
int parseLine(char *line, ADSBPacket *packet, Node **aircraftList, int *numPlanes) {
    Node* existingNode = NULL;

    //case that an estimated time is requested  
    if (sscanf(line, "*time:%d:%d,est_pos,%d", &packet->hours, &packet->minutes, &packet->id) == 3) {
        //time conversion
        packet->time = packet->hours * 3600 + packet->minutes * 60;
        //case that the plane is not in defined area
        existingNode = findNode(aircraftList, packet->id);
        if (existingNode == NULL) {
            printf("Aircraft (ID:%d) not currently in area of operation\n", packet->id);
            return 0;
        } 
        else {
            //prints estimated position
            EstimatePosition(aircraftList, *packet, true); //    printing
            return 0;
        }
    } 
    // case that a new node input is scaned
    else if (sscanf(line, "#ADS-B:%d,time:%d:%d,N:%lf,E:%lf,alt:%d,head:%lf,speed:%lf", &packet->id, &packet->hours, &packet->minutes, &packet->north, &packet->east, &packet->altitude, &packet->heading, &packet->speed) == 8) {
        // conversion to seconds
        packet->time = packet->hours * 3600 + packet->minutes * 60;
        //appending the data that we already have if ADSB number already exists
        existingNode = findNode(aircraftList, packet->id);
        if (existingNode == NULL) {
            appendNodes(aircraftList, *packet, numPlanes);
        }
        else {
            //creating new data if ADSB doesn't exist
            existingNode->data.time = packet->time;
            existingNode->data.hours = packet->hours;
            existingNode->data.minutes = packet->minutes;
            existingNode->data.north = packet->north;
            existingNode->data.east = packet->east;
            existingNode->data.altitude = packet->altitude;
            existingNode->data.heading = packet->heading;
            existingNode->data.hours = packet->hours;
            existingNode->data.speed = packet->speed;
        }
        Node* node = *aircraftList;
        while(node != NULL){
            node = node->next;
        }
    } 
    // if number of contacts is requested in the line
    else if(strstr(line, "num_contacts") != NULL){
        int hours;
        int minutes;
        // scanning time and converting to SI unit, seconds
        sscanf(line, "*time:%d:%d", &hours, &minutes);
        int seconds = hours*3600 + minutes*60;
        countPlanes(aircraftList, seconds);
        // calls num_plane function which works out number of planes in airspace
        return 1;
    }
    // checks if user would like to check the separation between aircrafts
    else if(sscanf(line, "*time:%d:%d,check_separation,%d,%lf", &packet->hours, &packet->minutes, &packet->id, &packet->min_sep) == 4) {
		int id = packet->id;
		int hours = packet->hours;
		int minutes = packet->minutes;
		double min_sep = packet->min_sep;

    	separationCheck(aircraftList, id, hours, minutes, min_sep);
        return 1;
    } else {
        // process of elimination with the commands. If all other statments do not work the function must be asking for the file to close.
        printf("closing\n");
        return 2; 
    }
    return 0;
}


// Function to free the memory of the linked list
void freeList(Node** aircraftList) {
    Node* current = *aircraftList;
    while (current != NULL) {
        //Node* toRemove = current;
        // moves on to next node when current node has been freed.
        current = current->next;
        //free(toRemove);
    }
}
//funciton counts the number of planes in the airspace
int countPlanes(Node** aircraftList, int time) {
    Node* current = *aircraftList;
    // initialising the count to 0
    int count = 0;

    while (current != NULL){
        ADSBPacket dataPacket = current->data;
        dataPacket.time = time;
        // function call to estimate position to check if aircraft is in range of airspace
        bool not_range = EstimatePosition(aircraftList, dataPacket, false);
        //remembers the node incase it needs to be removed

        Node *temp = current; 
        current = current-> next;

        if(not_range) {
            removeNode(aircraftList, temp->data.id);
        }
        else {
            if(dataPacket.id != 0){
                count++;
            }
        }
    }
    printf("Currently tracking %d aircraft\n", count);
    return count;
}

// Function to check separation between two aircraft
void separationCheck(Node** aircraftList, int id, int hours, int minutes, double min_sep) {
    double seconds = hours * 3600 + minutes * 60;
	Node *plane1_address = findNode(aircraftList, id);

	double plane1_north = calcNorth(seconds, plane1_address->data);
	double plane1_east = calcEast(seconds, plane1_address->data);
	double plane1_velocity_north = plane1_address->data.speed * sin(plane1_address->data.heading * M_PI / 180);
	double plane1_velocity_east = plane1_address->data.speed * cos(plane1_address->data.heading * M_PI / 180);

	double minTissue = -1;
	double nIssue;
	double eIssue;
    // Iterate over other aircraft in the list
    Node* node = *aircraftList;
    if (node->data.id != id) {
        while (node != NULL) {
			//Looks at the positions of a given plane and compare it to every plane in the linked list at time hour:minute using functions estimate_north() and estimate_east() to get north2r1, and east2r1.
			double plane2_north = calcNorth(seconds, node->data);
			double plane2_east = calcEast(seconds, node->data);
			double plane2_velocity_north = node->data.speed * sin(node->data.heading * M_PI / 180);
			double plane2_velocity_east = node->data.speed * cos(node->data.heading * M_PI / 180);

			double north2r1 = plane2_north - plane1_north;
			double east2r1 = plane2_east - plane1_east;
			double north2v1 = (plane2_velocity_north - plane1_velocity_north)/1000;
			double east2v1 = (plane2_velocity_east - plane1_velocity_east)/1000;
			//displacement2r1(t) = (north2r1 + north2v1 * t)i + (east2r1 + east2v1 * t)j
			//sqrt((north + north2v1 * timeDiff)^2 + (east2r1 + east2v1 * timeDiff)^2) < 100
			double a = north2v1*north2v1 + east2v1*east2v1;
			double b = 2*(north2v1*north2r1 + east2v1*east2r1);
			double c = north2r1*north2r1 + east2r1*east2r1 - min_sep*min_sep;
			double d = b*b - 4*a*c;

			//CHECK IF THE DISCRIMINANT IS POSITION
			
			if(d > 0){
				double tIssue = (-b-sqrt(d))/(2*a);
                //printf("tissue %lf", tIssue);
				if(tIssue > 0 && (tIssue < minTissue || minTissue == -1)){
					minTissue = tIssue;
					nIssue = calcNorth(tIssue + seconds, plane1_address->data);
					eIssue = calcEast(tIssue + seconds, plane1_address->data);
                
				}
			}
			node = node->next;
		}
	}
	if(minTissue == -1){
		printf("No separation issues\n");
	}else{
		printf("Separation issue: N:%.1lf,E:%.1lf\n", nIssue, eIssue);
	}
}

double calcNorth(double seconds, ADSBPacket plane) {

    int lastHour = plane.hours;
	int lastMinute = plane.minutes;
	double lastNorth = plane.north;
	double speed = plane.speed;
	double heading = plane.heading;
	double timeDiff = seconds - (lastHour*3600 + lastMinute*60);

	double north = lastNorth + speed/1000 * timeDiff * sin(heading * M_PI / 180);

	return north;
}

double calcEast(double seconds, ADSBPacket plane) {
    
    int lastHour = plane.hours;
	int lastMinute = plane.minutes;
	double lastEast = plane.east;
	double speed = plane.speed;
	double heading = plane.heading;
	double timeDiff = seconds - (lastHour*3600 + lastMinute*60);

	double east = lastEast + speed/1000 * timeDiff * cos(heading * M_PI / 180);
	
	return east;
}

