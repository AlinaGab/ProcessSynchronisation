ProcessSynchronisation
======================

A simple project for Process Synchronisation and Interprocess Communication.  Operating Systems course, Prof. Stathes Hadjiefthymiades.

OS 1st project

Alina Gabaraeva
AM 1115200800222

ΠΕΡΙΓΡΑΦΗ

Το πρόγραμμα υλοποιεί το πρόβλημα server-client με χρήση 2 διαμοιραζόμενων μνημών. Συγκεκριμένα, υπάρχει μια αρχική-γονική διεργασία η οποία γεννάει την διεργασία S. Η διεργασία C (γονική) κλωνοποιείται σε τυχαία χρονικά διαστήματα σε εκθετικό χρόνο. Οι κλώνοι C παράγουν αιτήσεις προς το πρός το S και βάζει τις εξής πληροφορίες στο in buffer: το pid της C'(που παράγει την αίτηση), το fileNo που θα διαχειρίζεται η S'. Με την παραλαβή του μηνύματος από το in buffer η διεργασία S φτίαχνει κλώνο S'. Ο κλώνος στην θέση του ανοίγει ένα αρχείο με το όνομα fileNo και αντλεί το περιεχόμενό του. Ύστερα το στέλνει στον out buffer και η διεργασία C' που έστελε το αίτημα παραλαμβάνει το μήνυμα και το γράφει στο logFILE. Στο logFile γράφεται και το realtime της υποβολής της αίτησης πρός την S' και το realtime της απάντησης που παραλαμβάνει η C' από το out buffer.

Ο υπολογισμός του realtime:



ΟΔΗΓΙΕΣ

για να τρέξετε το πρόγραμμα:
gcc proj1.c -lm
./a.out arg l k 

Όπου:
arg: αριθμός παραγόμενων διεργασιών C'/S'(ακέραιος)
l:   λ παράμετρος του υπολογισμού εκθετικής κατανομής
(Exponential time: http://stackoverflow.com/questions/2106503/pseudorandom-number-generator-exponential-distribution)
k:   πόσα έτοιμα αρχεία 80 char έχουμε στην διάθεσή μας

ΠΡΟΣΟΧΗ: To logFILE παραμένει αν ξαναεκτελεστεί το πρόγραμμα. Tα καινούρια δεδομένα θα μπούν στο ίδιο αρχείο log. Επομένως για επανεκτέλεση πρέπει να διαγραφεί.
Επίσης η C για να δημιουργεί κλώνους σε εκθετικό χρόνο πρέπει να πάρει αρκετά μικρό l, αλλιώς η μεταβλητή interval βγαίνει ίση με 0.0.


ΛΕΠΤΟΜΕΡΕΙΕΣ ΥΛΟΠΟΙΗΣΗΣ

Υλοποιήθηκαν συναρτήσεις για initialization σεμαφόρων (seminit), για να μπορεί να γίνει εύκολο up(up) και down(down) των system V σεμαφόρων. Οι διαμοιραζόμενες μνήμες είναι πακεταρισμένες σε 2 ξεχωριστά struct (struct in, struct out).

Αρχική διεργασία

Η αρχική διεργασία C γεννάει το παιδί της την S, όλα σε μια main. H C ύσερα θα περιμένει να τελειώσουν τα παιδιά της και η S που είναι και αυτή παιδί της.

Semaphores

sem_in: αρχικά=1
Γίνεται down στην C' πριν γράψει η C' στην shared memory in. 
Η διεργασία S' τον κάνει up μόλις διαβάσει από το in buffer.

sem_in2: αρχικά=0
Γίνεται up μόλις μπεί η πληροφορία από το C' στην in.
Η διεργασία S τον κάνει down πριν γεννήσει την καινούρια S'. Δεν θα μπορεί να γεννήσει αν δεν γίνει up από την C' (δηλαδή πρώτα πρέπει να υπάρχει πληροφορία στο in). 

sem_out: αρχικά=1
Γίνεται up από την C' μόλις διαβάσει από το out.
Γίνεται down από S' πρίν γράψει στο out.

sem_unique: αρχικά=0
Αρχικοποιείται και δημιουργείται στην διεργασία C'. Γίνεται up στην S' μετά που βάλει το μήμυμα στο out. Καταστρέφεται όταν τελειώσει η καθεμία συγκεκριμένη C' που δημιουργήθηκε απο την C, δηλαδή όταν πάρει την απαντησή της από το out buffer. Για να φαίνεται από την S' κάνω semget((key_t)Cpid,1,0) όπου Cpid είναι το pid της C' που έστειλε το αίτημα.

Shared Memory

Στη διαμοιραζόμενη μνήμη in υπάρχει το pid της C'(που παράγει την αίτηση) και το fileNo που θα διαχειρίζεται η S'. Στην out διαμοιζόμενη μνήμη έχουμε έναν πίνακα 80+1('/0') χαρακτήρων.

