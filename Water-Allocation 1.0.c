#include <stdio.h>
#include <string.h>

#define MAX_ZONES 50
#define MAX_NAME  40
#define EPSILON   0.0001


typedef struct {
    char  zoneID[10];
    char  serviceZone[MAX_NAME];
    char  type[25];
    double minRequirement;
    double lossPercent;
    int    prevWaitingCycles;

    double criticalityWeight;
    double priorityScore;
    int    priorityRank;

    double allocated;
    double effectiveDelivered;
    double shortage;
    char   condition[25];
} Zone;

/* ---------------------------- FUNCTION PROTOTYPES ------------------------- */
int    inputZones(Zone zones[]);
double inputAvailableWater(void);
double criticalityOf(const char *type);
void   computePriority(Zone zones[], int n);
void   sortByPriority(Zone zones[], int n);
double allocateWater(Zone zones[], int n, double available);
void   evaluateDelivery(Zone zones[], int n);
void   classifyCondition(Zone zones[], int n);
int    searchZoneByID(Zone zones[], int n, const char id[]);
int    indexOfHighestShortage(Zone zones[], int n);
void   printZoneTable(Zone zones[], int n);
void   printAllocationSummary(Zone zones[], int n, double totalAvailable, double waterLeft);

/* =========================== 1. INPUT FUNCTIONS =========================== */

double inputAvailableWater(void) {
    double water;
    printf("Enter total available potable water at the distribution centre (litres): ");
    scanf("%lf", &water);
    return water;
}

int inputZones(Zone zones[]) {
    int n;
    printf("Enter number of service zones (max %d): ", MAX_ZONES);
    scanf("%d", &n);
    if (n > MAX_ZONES) n = MAX_ZONES;

    for (int i = 0; i < n; i++) {
        printf("\n--- Zone %d ---\n", i + 1);
        printf("Zone ID: ");
        scanf("%9s", zones[i].zoneID);
        printf("Service zone name: ");
        scanf(" %39[^\n]", zones[i].serviceZone);
        printf("Type (Hospital/EmergencyShelter/Residential/EmergencyService): ");
        scanf(" %24s", zones[i].type);
        printf("Requested water (L): ");
        scanf("%lf", &zones[i].requested);
        printf("Minimum requirement (L): ");
        scanf("%lf", &zones[i].minRequirement);
        printf("Distribution loss (%%): ");
        scanf("%lf", &zones[i].lossPercent);
        printf("Previous waiting cycles: ");
        scanf("%d", &zones[i].prevWaitingCycles);

        zones[i].allocated = 0.0;
        zones[i].effectiveDelivered = 0.0;
        zones[i].shortage = 0.0;
        strcpy(zones[i].condition, "Unserved");
    }
    return n;
}


double criticalityOf(const char *type) {
    if (strcmp(type, "Hospital") == 0)          return 40.0;
    if (strcmp(type, "EmergencyService") == 0)  return 35.0;
    if (strcmp(type, "EmergencyShelter") == 0)  return 30.0;
    if (strcmp(type, "Residential") == 0)       return 20.0;
    return 15.0;
}

void computePriority(Zone zones[], int n) {
    for (int i = 0; i < n; i++) {
        zones[i].criticalityWeight = criticalityOf(zones[i].type);

        zones[i].priorityScore = zones[i].criticalityWeight
                                  + (5.0 * zones[i].prevWaitingCycles);
    }
}

                             */
static int higherPriority(const Zone *a, const Zone *b, int aOriginalPos, int bOriginalPos) {
    if (a->priorityScore != b->priorityScore)
        return a->priorityScore > b->priorityScore;
    if (a->prevWaitingCycles != b->prevWaitingCycles)
        return a->prevWaitingCycles > b->prevWaitingCycles;
    double aUnresolved = a->requested - a->minRequirement;
    double bUnresolved = b->requested - b->minRequirement;
    if (aUnresolved != bUnresolved)
        return aUnresolved > bUnresolved;
    return aOriginalPos < bOriginalPos;
}

void sortByPriority(Zone zones[], int n) {
    int originalPos[MAX_ZONES];
    for (int i = 0; i < n; i++) originalPos[i] = i;

    for (int i = 0; i < n - 1; i++) {
        int best = i;
        for (int j = i + 1; j < n; j++) {
            if (higherPriority(&zones[j], &zones[best], originalPos[j], originalPos[best]))
                best = j;
        }
        if (best != i) {
            Zone tmp = zones[i]; zones[i] = zones[best]; zones[best] = tmp;
            int t = originalPos[i]; originalPos[i] = originalPos[best]; originalPos[best] = t;
        }
    }
    for (int i = 0; i < n; i++) zones[i].priorityRank = i + 1;
}

/
double allocateWater(Zone zones[], int n, double available) {
    /* Phase 1: minimum guarantee, released amount adjusted for loss */
    for (int i = 0; i < n && available > EPSILON; i++) {
        double factor = 1.0 - (zones[i].lossPercent / 100.0);
        double releaseForMin = zones[i].minRequirement / factor;

        if (available >= releaseForMin) {
            zones[i].allocated = releaseForMin;
            available -= releaseForMin;
        } else {
            zones[i].allocated = available;
            available = 0.0;
        }
    }


    for (int i = 0; i < n && available > EPSILON; i++) {
        double factor = 1.0 - (zones[i].lossPercent / 100.0);
        double releaseForMin = zones[i].minRequirement / factor;

        if (zones[i].allocated + EPSILON < releaseForMin)
            continue;

        double releaseForFull = zones[i].requested / factor;
        double extraNeeded = releaseForFull - zones[i].allocated;
        if (extraNeeded <= EPSILON) continue;

        if (available >= extraNeeded) {
            zones[i].allocated += extraNeeded;
            available -= extraNeeded;
        } else {
            zones[i].allocated += available;
            available = 0.0;
        }
    }

    if (available < 0) available = 0;
    return available;
}

void evaluateDelivery(Zone zones[], int n) {
    for (int i = 0; i < n; i++) {
        double factor = 1.0 - (zones[i].lossPercent / 100.0);
        zones[i].effectiveDelivered = zones[i].allocated * factor;
        zones[i].shortage = zones[i].requested - zones[i].effectiveDelivered;
        if (zones[i].shortage < 0) zones[i].shortage = 0;
    }
}

void classifyCondition(Zone zones[], int n) {
    for (int i = 0; i < n; i++) {
        if (zones[i].effectiveDelivered <= EPSILON)
            strcpy(zones[i].condition, "Unserved");
        else if (zones[i].effectiveDelivered + EPSILON >= zones[i].requested)
            strcpy(zones[i].condition, "FullRequirement");
        else if (zones[i].effectiveDelivered + EPSILON >= zones[i].minRequirement)
            strcpy(zones[i].condition, "MinimumSatisfied");
        else
            strcpy(zones[i].condition, "BelowMinimum");
    }
}

int searchZoneByID(Zone zones[], int n, const char id[]) {
    for (int i = 0; i < n; i++)
        if (strcmp(zones[i].zoneID, id) == 0)
            return i;
    return -1;
}

int indexOfHighestShortage(Zone zones[], int n) {
    int idx = 0;
    for (int i = 1; i < n; i++)
        if (zones[i].shortage > zones[idx].shortage)
            idx = i;
    return idx;
}

void printZoneTable(Zone zones[], int n) {
    printf("\n%-4s %-22s %-9s %-9s %-9s %-9s %-9s %-16s\n",
           "Rank", "Service Zone", "Reqstd", "MinReq", "Alloc", "EffDlv", "Shortage", "Condition");
    printf("---------------------------------------------------------------------------------------------\n");
    for (int i = 0; i < n; i++) {
        printf("%-4d %-22s %-9.1f %-9.1f %-9.1f %-9.1f %-9.1f %-16s\n",
               zones[i].priorityRank, zones[i].serviceZone, zones[i].requested,
               zones[i].minRequirement, zones[i].allocated, zones[i].effectiveDelivered,
               zones[i].shortage, zones[i].condition);
    }
}

void printAllocationSummary(Zone zones[], int n, double totalAvailable, double waterLeft) {
    double totalRequested = 0, totalAllocated = 0, totalEffective = 0, totalShortage = 0;
    int full = 0, minOnly = 0, belowMin = 0, unserved = 0;

    for (int i = 0; i < n; i++) {
        totalRequested += zones[i].requested;
        totalAllocated += zones[i].allocated;
        totalEffective += zones[i].effectiveDelivered;
        totalShortage  += zones[i].shortage;

        if (strcmp(zones[i].condition, "FullRequirement") == 0) full++;
        else if (strcmp(zones[i].condition, "MinimumSatisfied") == 0) minOnly++;
        else if (strcmp(zones[i].condition, "BelowMinimum") == 0) belowMin++;
        else unserved++;
    }

    printf("\n================= SUMMARY OF ALLOCATION =================\n");
    printf("Zone ID | Service Zone            | Requested | MinReq | Priority | Allocated | EffDelivered | Shortage | Condition\n");
    for (int i = 0; i < n; i++) {
        printf("%-7s | %-24s | %-9.1f | %-6.1f | %-8d | %-9.1f | %-12.1f | %-8.1f | %s\n",
               zones[i].zoneID, zones[i].serviceZone, zones[i].requested,
               zones[i].minRequirement, zones[i].priorityRank, zones[i].allocated,
               zones[i].effectiveDelivered, zones[i].shortage, zones[i].condition);
    }

    printf("\nTotal water available at start      : %.1f L\n", totalAvailable);
    printf("Total water requested                : %.1f L\n", totalRequested);
    printf("Total water allocated (released)     : %.1f L\n", totalAllocated);
    printf("Total effective water delivered      : %.1f L\n", totalEffective);
    printf("Total unresolved requirement         : %.1f L\n", totalShortage);
    printf("Remaining water at distribution centre: %.1f L\n", waterLeft);
    printf("Zones fully satisfied                : %d\n", full);
    printf("Zones with only minimum satisfied    : %d\n", minOnly);
    printf("Zones below minimum requirement      : %d\n", belowMin);
    printf("Zones completely unserved            : %d\n", unserved);

    if (totalAllocated > totalAvailable + EPSILON)
        printf("\n*** WARNING: allocation exceeds available supply - logic error! ***\n");
    else
        printf("\nCheck: total allocated (%.1f) does not exceed available supply (%.1f). OK.\n",
               totalAllocated, totalAvailable);
}

/* ================================= MAIN ==================================== */
int main(void) {
    Zone zones[MAX_ZONES];
    int n;
    double totalAvailable, waterLeft;
    int choice;
    char searchID[10];

    printf("=== Emergency Potable Water Allocation System ===\n\n");

    totalAvailable = inputAvailableWater();
    n = inputZones(zones);

    computePriority(zones, n);
    sortByPriority(zones, n);
    waterLeft = allocateWater(zones, n, totalAvailable);
    evaluateDelivery(zones, n);
    classifyCondition(zones, n);

    printf("\n--- Zones ordered by allocation priority ---");
    printZoneTable(zones, n);

    printAllocationSummary(zones, n, totalAvailable, waterLeft);

    int hi = indexOfHighestShortage(zones, n);
    printf("\nZone with the highest unresolved requirement: %s (%s) - shortage %.1f L\n",
           zones[hi].zoneID, zones[hi].serviceZone, zones[hi].shortage);

    printf("\nSearch for a zone by Zone ID? (1 = Yes, 0 = No): ");
    scanf("%d", &choice);
    if (choice == 1) {
        printf("Enter Zone ID to search: ");
        scanf("%9s", searchID);
        int idx = searchZoneByID(zones, n, searchID);
        if (idx == -1) {
            printf("Zone %s not found.\n", searchID);
        } else {
            printf("\n--- Details for %s ---\n", zones[idx].zoneID);
            printf("Service Zone      : %s\n", zones[idx].serviceZone);
            printf("Type              : %s\n", zones[idx].type);
            printf("Requested         : %.1f L\n", zones[idx].requested);
            printf("Minimum Required  : %.1f L\n", zones[idx].minRequirement);
            printf("Priority Rank     : %d\n", zones[idx].priorityRank);
            printf("Allocated         : %.1f L\n", zones[idx].allocated);
            printf("Effective Delivered: %.1f L\n", zones[idx].effectiveDelivered);
            printf("Shortage          : %.1f L\n", zones[idx].shortage);
            printf("Condition         : %s\n", zones[idx].condition);
        }
    }

    return 0;
}
