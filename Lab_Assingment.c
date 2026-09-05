#include <stdio.h>
#include <string.h>

#define MAX_ZONES 50
#define MAX_NAME  40
#define EPSILON   0.0001


typedef struct {
    char  zoneID[10];
    char  serviceZone[MAX_NAME];
    char  type[25];
    double requested;
    double minRequirement;
    double lossPercent;
    int    prevWaitingCycles;

    double criticalityWeight;
    double priorityScore;
    int    priorityRank;

    double allocated;
    double effectiveDelivered;
    double shortage;
    char   condition[15];
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
void   printWithCommas(double value);
void   printBanner(const char *caseLabel, const char *datasetDesc, double totalAvailable);
void   printAllocationResult(Zone zones[], int n);
void   printPriorityOrder(Zone zones[], int n);
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

/* ===================== 2. PRIORITY DETERMINATION (Task 1 & 3) =============

   ============================================================================ */
double criticalityOf(const char *type) {
    if (strcmp(type, "Hospital") == 0)          return 40.0;
    if (strcmp(type, "EmergencyService") == 0)  return 35.0;
    if (strcmp(type, "EmergencyShelter") == 0)  return 30.0;
    if (strcmp(type, "Residential") == 0)       return 20.0;
    return 15.0; /* unknown / other type -> lowest default weight */
}

void computePriority(Zone zones[], int n) {
    for (int i = 0; i < n; i++) {
        zones[i].criticalityWeight = criticalityOf(zones[i].type);

        zones[i].priorityScore = zones[i].criticalityWeight
                                  + (5.0 * zones[i].prevWaitingCycles);
    }
}


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

/* ===================== 3. ALLOCATION (Task 3 & 4) =========================

   ============================================================================ */
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

/* ===================== 4. DISTRIBUTION LOSS / EFFECTIVE DELIVERY ========== */
void evaluateDelivery(Zone zones[], int n) {
    for (int i = 0; i < n; i++) {
        double factor = 1.0 - (zones[i].lossPercent / 100.0);
        zones[i].effectiveDelivered = zones[i].allocated * factor;
        zones[i].shortage = zones[i].requested - zones[i].effectiveDelivered;
        if (zones[i].shortage < 0) zones[i].shortage = 0;
    }
}

/* ===================== 5. SEARCHING / CLASSIFICATION ======================= */
void classifyCondition(Zone zones[], int n) {
    for (int i = 0; i < n; i++) {
        if (zones[i].effectiveDelivered <= EPSILON)
            strcpy(zones[i].condition, "UNSERVED");
        else if (zones[i].effectiveDelivered + EPSILON >= zones[i].requested)
            strcpy(zones[i].condition, "FULL");
        else if (zones[i].effectiveDelivered + EPSILON >= zones[i].minRequirement)
            strcpy(zones[i].condition, "MINIMUM");
        else
            strcpy(zones[i].condition, "BELOW MIN");
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

/* ============================ OUTPUT / REPORTS ============================

   ============================================================================ */


void printWithCommas(double value) {
    long whole = (long)(value + 0.5);
    char buf[32];
    sprintf(buf, "%ld", whole);
    int len = strlen(buf);
    int firstGroup = len % 3;
    if (firstGroup == 0) firstGroup = 3;
    for (int i = 0; i < len; i++) {
        if (i != 0 && (i - firstGroup) % 3 == 0) putchar(',');
        putchar(buf[i]);
    }
}

void printBanner(const char *caseLabel, const char *datasetDesc, double totalAvailable) {
    printf("\n================================================================================\n");
    printf("                     EMERGENCY POTABLE WATER ALLOCATION\n");
    printf("================================================================================\n");
    printf("\n========== %s ==========\n", caseLabel);
    printf("%s\n", datasetDesc);
    printf("Available Water: ");
    printWithCommas(totalAvailable);
    printf(" L\n");
}

void printAllocationResult(Zone zones[], int n) {
    printf("\n================================================================================\n");
    printf("                                ALLOCATION RESULT\n");
    printf("================================================================================\n");
    printf("%-4s %-24s %-9s %-9s %-9s %-10s %-10s %-9s %-10s\n",
           "ID", "Zone", "Request", "Minimum", "Priority", "Allocated", "Delivered", "Shortage", "Status");
    printf("--------------------------------------------------------------------------------------------\n");
    for (int i = 0; i < n; i++) {
        printf("%-4s %-24s %-9.0f %-9.0f %-9.2f %-10.0f %-10.0f %-9.0f %-10s\n",
               zones[i].zoneID, zones[i].serviceZone, zones[i].requested, zones[i].minRequirement,
               zones[i].priorityScore, zones[i].allocated, zones[i].effectiveDelivered,
               zones[i].shortage, zones[i].condition);
    }
    printf("================================================================================\n");
}

void printPriorityOrder(Zone zones[], int n) {
    printf("\nPriority Order:\n");
    for (int i = 0; i < n; i++) {
        printf("%d. %s - %s (Priority %.2f)\n",
               zones[i].priorityRank, zones[i].zoneID, zones[i].serviceZone, zones[i].priorityScore);
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

        if (strcmp(zones[i].condition, "FULL") == 0) full++;
        else if (strcmp(zones[i].condition, "MINIMUM") == 0) minOnly++;
        else if (strcmp(zones[i].condition, "BELOW MIN") == 0) belowMin++;
        else unserved++;
    }

    printf("\nSummary\n");
    printf("------------------------------------------\n");
    printf("Total Requested Water          : %.2f L\n", totalRequested);
    printf("Total Allocated Water          : %.2f L\n", totalAllocated);
    printf("Total Effective Water Delivered: %.2f L\n", totalEffective);
    printf("Total Unresolved Requirement   : %.2f L\n", totalShortage);
    printf("Remaining Water                : %.2f L\n", waterLeft);

    printf("\nService Conditions\n");
    printf("------------------------------------------\n");
    printf("Full Requirement Satisfied   : %d zone(s)\n", full);
    printf("Minimum Requirement Satisfied: %d zone(s)\n", minOnly);
    printf("Below Minimum                : %d zone(s)\n", belowMin);
    printf("Completely Unserved          : %d zone(s)\n", unserved);

    if (totalAllocated > totalAvailable + EPSILON)
        printf("\n*** WARNING: allocation exceeds available supply - logic error! ***\n");
    else
        printf("\nCheck: total allocated (%.2f L) does not exceed available supply (%.2f L). OK.\n",
               totalAllocated, totalAvailable);
}

/* ================================= MAIN ==================================== */
int main(void) {
    Zone zones[MAX_ZONES];
    int n;
    double totalAvailable, waterLeft;
    int choice;
    char searchID[10];
    char caseLabel[50], datasetDesc[100];

    printf("Enter test case label (e.g. CASE 1): ");
    scanf(" %49[^\n]", caseLabel);
    printf("Enter dataset description (e.g. Original Dataset): ");
    scanf(" %99[^\n]", datasetDesc);

    totalAvailable = inputAvailableWater();
    n = inputZones(zones);

    computePriority(zones, n);
    sortByPriority(zones, n);
    waterLeft = allocateWater(zones, n, totalAvailable);
    evaluateDelivery(zones, n);
    classifyCondition(zones, n);

    printBanner(caseLabel, datasetDesc, totalAvailable);
    printAllocationResult(zones, n);
    printPriorityOrder(zones, n);
    printAllocationSummary(zones, n, totalAvailable, waterLeft);

    int hi = indexOfHighestShortage(zones, n);
    printf("\nZone with the highest unresolved requirement: %s (%s) - shortage %.2f L\n",
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
            printf("Requested         : %.2f L\n", zones[idx].requested);
            printf("Minimum Required  : %.2f L\n", zones[idx].minRequirement);
            printf("Priority Rank     : %d\n", zones[idx].priorityRank);
            printf("Allocated         : %.2f L\n", zones[idx].allocated);
            printf("Effective Delivered: %.2f L\n", zones[idx].effectiveDelivered);
            printf("Shortage          : %.2f L\n", zones[idx].shortage);
            printf("Condition         : %s\n", zones[idx].condition);
        }
    }

    return 0;
}
