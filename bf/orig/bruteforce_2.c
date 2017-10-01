/*
 Programmed by freax
 Simple bruteforce crack algoritm
 http://users.pandora.be/skatan

 First of all, this program or tool is for educational purpose only.
 Trying to crack/hack/blahblah/mastrubate when using ms/DoS flooding/
 other people is stupid, lame, sick, idiot,.. you name it... So don't. 
 Using this tool won't make you superman, hackingwarface or z3r0|{00l 
 the hackmachine. Nor did programming this do to me. I am just another 
 lame c newbie trying to learn some c programming and some linux.. so 
 also don't ask me how to use this tool.. I have no f+ck+ng idea.
 It should generate an output list with all the possible character 
 combinations.. so a bruteforce or however you wan't to call it.
 I call it bruteforce, maybe you don't, well .. so what ! The point is..
 it does what it is suppose to do.. generate stuff.. narghh passwords
 if you wan't to call it passwords. I don't care
 
 Afther you reallyreally red the `first of all' part , my mother/home 
 language is not English. And my teacher English .. oh well, I just 
 never pay attention during his classes .. so my English s+cks, sorry.. 
 I care, but not yet when I am programming or typing comments in my 
 programs.

 So eum.. for the stupid childer who downloaded this and don't know 
 how to compile things (get a compiler here : www.gnu.org)
  To compile : gcc -o bruteforce bruteforce.c
  To use : see bruteforce --help
  Tip : use '>' and '|' to pipe the output
 
  Example, a wordlist of all possible password combinations 
  with 5 chars and lowercase characters only): 
  ~$ bruteforce -s 97 -e 122 -p 5 > words.txt
  ~$ grep linux words.txt
  linux
  ~$

  Ttip :his tool will help you a lot for the -s and -e paramters :
      http://users.pandora.be/skatan/files/ascii.c  

*/

// global variables declaration
// t[255] means that the max size of the password is 255 characters !!
// If it's to big anyway : make it smaller (maybe the code will run
// faster ?! Less memory eh...). It's up to you.
int t[255], maxd, maxv, x, y, startwith;

print_version_info() {
// No don't change this part =) it's stable .. lol
 printf("%s","Bruteforce crack algoritm by freax");
 printf("%s\n"," v1.0stable");
}

print_parameter_info(char *prog_n) {
// doh, I forgot what this function does
 printf("\n");
 printf("%s%s%s\n","usage: ",prog_n," -p .. [-s ..] [-e ..] [-v]");
 printf("%s\n","  --password-size or -p ..  : size of the password");
 printf("%s\n","  --start-with or -s ..     : start code (32)");
 printf("%s\n","  --end-with or -e ..       : end code (126)");
 printf("%s\n","  --version or -v           : version info");
 printf("%s\n","  --help or -h              : help - this");
 printf("%s\n","  --info or -i              : output list info");
 printf("\n");
}

// The mileage counter , if you know a faster way.. please let me know
// So we use a mileage counter where the max value per digit is [end code]
// We count between [start code] and [end code] and we print the integers
// as characters (%c). The amount of digits is the size of the password
// of course.

brute_force() {					// bo brute_force()
for (y = 0; y <= maxd; ++y) t[y] = startwith; 	// Reset the array
t[maxd] = startwith; 				// Set the last number of the array
while (t[maxd] < startwith+1) {			// bo loop: Until the last number is not 1 more then startwith
 ++t[0];					// Increase the first number of the array
 for (x = 0; x <= maxd; ++x) {  		// be loop: Check the whole array (a lot unnessesairy loops, fix this plz)
  if (t[x] > maxv) {				// bo If: the current number > maxvalue
   t[x] = startwith;				  // the current number = startwith
  ++t[x+1];					  // increase the next number
  }						  // eo if: t[x]>maxv
 }						// eo loop: for-loop
// For piping, put your other output here 
// printf("%s\n","root"); // =)
for (y=0;y<maxd;++y)  printf("%c",t[y]);
// Change this if you would like to use another spacer between the 
// found passwords/character lists. 
printf("\n");
}						// eo loop: while-loop

/* Fix this hun:
    for (x = 0; x <= maxd; ++x) loops maxd times while it only
    should loop (current position + 1) times. The code will run
    a lot faster I think. (-50% irritations?!).
*/
}						// eo brute_force()


// Main function with parameter declarations
main(int argc, char *argv[]) {			// bo main()
 int p;
 maxd = 0;
 // standard values
 maxv = 126;
 startwith = 32; 

 // if there are paramters
 if (argc > 1) {
  for (p=1;p<argc;++p) {
 // The parameter checking
   if (!strcmp(argv[p], "--help")) { print_parameter_info(argv[0]); exit(1); };
   if (!strcmp(argv[p], "-h")) { print_parameter_info(argv[0]); exit(1); };
   if (!strcmp(argv[p], "--version")) { print_version_info(); exit(1); };
   if (!strcmp(argv[p], "-v")) { print_version_info(); exit(1); };
   if (!strcmp(argv[p], "--password-size")) maxd= atoi(argv[p+1]);
   if (!strcmp(argv[p], "-p")) maxd= atoi(argv[p+1]);
   if (!strcmp(argv[p], "--start-with")) startwith= atoi(argv[p+1]);
   if (!strcmp(argv[p], "-s")) startwith = atoi(argv[p+1]);
   if (!strcmp(argv[p], "--end-with")) maxv = atoi(argv[p+1]);
   if (!strcmp(argv[p], "-e")) maxv = atoi(argv[p+1]);
   if (!strcmp(argv[p], "-i")) printf("%10s%c%5d%2c%8s%c%5d%2c%13s%c%5d\n","Start code",':',startwith,' ',"End code",':',maxv,' ',"Password size",':',maxd);
   if (!strcmp(argv[p], "--info")) printf("%10s%c%5d%2c%8s%c%5d%2c%13s%c%5d\n","Start code",':',startwith,' ',"End code",':',maxv,' ',"Password size",':',maxd);
  }
 }
 else {  
 // if there are no paramters (oeps .. user forgot password size)
  print_version_info();
  print_parameter_info(argv[0]);
  exit(1);
 } 

 if ((maxd != 0) && (startwith != 0) && (maxv != 0) ) { 
 // If everything is o.k. we start the bruteforce mileage counter
  brute_force(); 

 } else {
 // else not :) (so maxd == 0)
  print_version_info();
  print_parameter_info(argv[0]);
  exit(1);
 }
} 						// eo main()