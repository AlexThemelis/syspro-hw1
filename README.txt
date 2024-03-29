Για τον sniffer:
Compilation: make
Clean των executable: make clean
Run: ./sniffer [-p path] (Γίνεται έλεγχο ότι έχει δωθεί σωστό input)
(Το path θεωρείται οτι δίνεται σωστά και με / στο τέλος π.χ. /Desktop/ και οχι /Desktop)

Για τις δημιουργίες των οντοτήτων Listener και Manager κάνω ενα fork() στο οποίο
το παιδί έχει ρόλο του Listener και ο γονιός έχει τον ρόλο του Manager.

Ο Listener αρχικά κλείνει το READ end του pipe αφού δεν θα το χρησιμοποιήσει
και κάνει ανακατεύθυνση το standard output του στο WRITE end του pipe. Ετσι,
κάνοντας μετά execl στην Inotify, ο Listener θα έχει επικοινωνία με τον Manager
ακόμα και αν θεωρητικά, ο Listener μετα το exec "καταστρέφεται" και περνάει η Inotifywait.
To path της Inotifywait σε κάθε μηχάνημα μπορούμε να το πάρουμε με which Inotifywait 
(εδώ το έχουμε hardcoded αφού θέλουμε να τρέχει στα linux του di). Εκτός από το όρισμα
-m (για να τρέχει ατέρμονα) περνάμε και τα ορίσματα -e create -e moved_to. Αυτό το 
κάνουμε ώστε να απομονώσουμε τις πραγματικές πληροφορίες με των "θόρυβο" που εκτυπώνει
η Inotifywait. Μπορούμε να δούμε από το man page της Inotifywait ότι μας ενδιαφέρουν 
μόνο αυτά τα 2 events, create και moved_to. 

Ο Manager τώρα αρχίζει ενα infinite loop οπού διαβάζει από το pipe που μοιράζεται με 
τον Listener. Έτσι, κάθε φορα μπλοκάρει και περιμένει ο Listener να του στείλει κατι για
να το διαβάσει. Μετά, γίνονται 2 regex searches για να δουμε αν ο Listnere έστειλε create
ή moved_to ειδοποίηση και απομονώνουμε έτσι το filename που άλλαξε στο directory που μας
ενδιαφέρει. Έπειτα, ο Manager κοιτάει αν η ουρά με τους διαθέσιμους workers είναι κενή. 
Σε αυτήν την ουρά θα βάζουμε όσους workers εχουν δημιουργηθεί και έχουν τελειώσει την 
εργασία που τους είχε ανατεθεί. Φυσικά, την πρώτη φορά αυτη η ουρά θα είναι κενή. Έτσι,
ο Manager θα κάνει άλλο ενα fork() στο παιδί, θα δημιουργήσει ενα named Pipe (Fifo) με το όνομα
του PID του παιδιού. Αυτό το κάνουμε καθώς η exec που θα κάνουμε προσεχώς δεν αλλάζει το PID της διεργασίας
και έτσι θα μπορούμε εύκολα να συνδέουμε worker με το named pipe του. Αφού δημιουργήσει το 
named pipe, το παιδί θα κάνει exec για να "γίνει" worker. Ο γονέας εκείνου του fork() θα ανοίξει
το pipe έχοντας το pid του παιδιού του και θα περάσει το όνομα του αρχέιου που ο Listener ανέφερε. 
Σημειώνω, ότι αν στα αρχικά ορίσματα του sniffer τρέχαμε με -p, θα βάλουμε και το path πίσω στο filename
ώστε να ξέρει και το path ο Worker. 

Ο Worker τώρα όταν δημιουργείται ανοίγει το named pipe του και πηγαίνει να διαβάσει το filename. 
Έπειτα δημιουργεί ένα αρχείο με μορφή <filename>.out και αρχίζει να διαβάσει απο το αρχείο που έχει
το filename. Σε κάθε loop της read(), καλείται η συνάρτηση url_extracter() η οποία θα "επιστρέψει" μέσω ενός 
"μεγάλου" στατικού πίνακα που περνάμε την διεύθυνση όλα τα urls που υπήρχαν στον buffer. H url_extracter()
μέσω regex απομονώνει τα urls και επιστρέφει πόσα βρήκε. Έπειτα, αφαιρώ διάφορα όπως το http:// κλπ μέχρι
να έχουμε το url στην επιθυμητή μορφή του. Αυτά τα urls τα κρατάμε και τα γράφουμε στο αρχείο που είχε δημιουργήσει
ο worker πιο πριν, μαζί με το πόσες φορές υπήρχε. Τέλος, ο worker αφού έχει την εργασία που του έχει ανατεθεί
κάνει raise(SIGSTOP), στέλνοντας ένα σήμα SIGSTOP στον εαυτό του.

Ο Manager, κάνοντας το παιδί(worker) raise(SIGSTOP) δέχεται ένα σήμα SIGCHLD. Γενικά αυτο το σήμα αγνοείται όμως 
εδώ έχουμε δηλώσει έναν ειδικό signal handler για αυτό το σήμα. Σε αυτόν τον signal handler, ο manager κάνει waitpid() 
με flags WNOHANG και WUNTRACED και περιμένει να επιστραφεί το pid του παιδιού που άλλαξε κατάσταση. Έτσι, παίρνεi
το pid του worker που έκανε raise() και τον βάζει στην ουρά των διαθέσιμων workers. 

Τώρα, αν η ουρά δεν ήταν κενή στον manager, θα παίρναμε τον πρώτο διαθέσιμο worker
στην σειρά και θα του κάναμε στέλναμε SIGCONT για να συνέχιζε, αφού είχε σταματήσει με SIGSTOP.
Σημαντικό εδώ είναι ότι επειδή αν στείλουμε SIGCONT στο παιδί πάλι θα έπρεπε να γίνει handle του SIGCHLD
σήματος που θα υπήρχε, πριν το kill() αλλάζω τον signal handler του SIGCHLD σε null handler (δηλαδή να το
κάνει ignore). Μετά το kill(), ξαναβάζω τον signal handler να είνα όπως είχαμε δηλώσει πριν. Γνωρίζω ότι 
αυτή η λογική με το να αλλάζω του signal handlers είναι επικύνδινη καθώς θα μπορούσε να χαθεί κάποιο σήμα
και ότι πιθανώς μπορεί να υπάρχει κάποιος καλύτερος τρόπος. Έπειτα, ο manager θα ανοίξει ξανα το named pipe 
για να στείλει στον worker το καινούργιο filename που πρέπει να ψάξει.

Αυτό συνεχίζεται ατέρμονα μέχρι ο χρήστης να δώσει σήμα SIGINT (ctrl-c). Έχουμε φτιάξει έναν signal handler 
για το SIGINT o οποίος για κάθε worker στην ουρά των διαθέσιμων workers, διαγράφει το named pipe που ήταν 
για αυτόν, σκοτώνει τον worker και κάνει free το pid του. Έπειτα, σκοτώνει τον listener με SIGKILL (το pid
του listener το έχουμε κρατήσει με μια global μεταβλητή) και τέλος κάνει exit για να τελειώσει ο sniffer.


Notes:
-Για τα διάφορα parsing των urls (και γενικά των strings) χρησιμοποιώ την βιβλιοθήκη regex. 
       
- Τα named Pipes και τα .out αρχεία δημιουργούνται στο /tmp/ (είναι και defined ως OUTPUTPATH)

-Στην παρούσα εργασία έχουν παρθεί κάποιες παραδοχές. Αρχικά, η πρώτη παραδοχή είναι ότι σε κάθε αρχείο
ο μέγιστος αριθμός url που θα μπορεί να έχει ειναι 250. Αυτό υπάρχει ώστε να μπορεί να φτιαχτεί ένας στατικός
πίνακας για να μετράει πόσες φορές έχει εμφανιστεί κάποιο url. Μια λύση που είχα σκεφτεί αλλά δεν την υλοποίησα
λόγω χρόνου είναι να εκτυπώνω 1-1 τα url στο αρχείο .out (όπου δεν χρειάζεται στατικός πίνακας) και έπειτα να
έφτιαχνα μια συνάρτηση η οποία να κάνει parse το .out αρχείο και να το φτιάχνει στην τελική μορφή που ζητείται
από την άσκηση. Μια δεύτερη παραδοχή είναι ότι όταν ο χρήστης δώσει SIGINT (ctrl-c) θα έχουν τελειώσει όλοι οι worker
και έτσι μπορώ να διαγράφω τα named pipes από την ουρά. Η τρίτη παραδοχή είναι ότι πρακτικά κανένα url δεν θα 
"σπάσει" στην μέση με τα διαβάσματα του buffer. Επειδή ο buffer διαβαζει 8192 bytes ανα φορά, θα μπορούσε να "σπάσει" στα 2
ενα url με αποτέλεσμα να "χαθεί" αυτό το url. 

-Γίνονται γενικά έλεγχοι για τις συναρτήσεις αν τρέχουν σωστά 
και υπάρχουν τα κατάλληλα perror και exit() σε αντίθετη περίπτωση.

Για το bash script:
Run: ./finder.sh TLD1 [TLD2 TLD3 ....] (Γίνεται έλεγχος ότι έχει δωθεί τουλάχιστον ένα TLD)
(Τα TLD θεωρούνται οτι δίνονται χωρίς την τελεία (.) δηλαδη "com" και οχι ".com") 

Στο script κάνω μια επανάληψη όλων των TLD ορισμάτων. Σε αυτήν την επανάληψη παίρνω ένα-ένα
τα files που βρίσκονται στο /tmp/* (hardcoded, είναι το path το οποίο ο sniffer βάζει τα 
.out αρχεία). Εκεί σε κάθε όνομα αρχείου κρατάω τους τελευταίους 4 χαρακτήρες και ελέγχω με
ένα if αν ειναι ".out" δηλαδή αν το αρχείο είναι της μορφής <filename>.out . Έπειτα κάνω μια
`grep -c .$TLD[[:space:]] $file` και ελέγχω αν το αποτέλεσμα είναι 0. Αυτό γίνεται διότι αν
είναι 0, στις παρακάτω εντολές (με την awk αθροίζοντας) δεν παίρνω αριθμό και το ολικό άθροισμα
χτυπάει error. Για να αθροίσω τις εμφανίσεις των url μέσω της `grep .$TLD[[:space:]] $file`
παίρνω όλες τις φορές που το TLD με . πίσω του και μετα κενό (αυτο γίνεται ώστε να μην πιάσει
το amazon.es.com ως es). Έπειτα κάνω pipe με την `awk '{SUM+=$2}END{print SUM}'` που παίρνει 
την 2η στήλη (από αυτα που είχαμε βρεί με την grep) και την αθροίζει, δηλαδή αθροίζει τον αριθμό
εμφανίσεων. Έπειτα συνεχίζεται αυτό σε όλα τα άλλα files και όταν τελειώσουν, εκτυπώνει το αποτέλεσμα
και συνεχίζει στο επόμενο TLD.