/*
IMPORTANT NOTE about TembooAccount.h

TembooAccount.h contains your Temboo account information and must be included
alongside your sketch. To do so, make a new tab in Arduino, call it TembooAccount.h,
and copy this content into it. 
*/

// Note that for additional security and reusability, you could
// use #define statements to specify these values in a .h file.

#define GOOGLE_CLIENT_ID "383524131748-qcj01cfos7cqr3ev92ofvce563q7ceec.apps.googleusercontent.com"
#define GOOGLE_CLIENT_SECRET "dQUfsvVLi1AiLjrLps4SLcsg"
#define GOOGLE_REFRESH_TOKEN "1/uL4I1IAWw8Q0VbH2ePuTCFSJkCE771uCtT_GKZAN5Cc"

#define TEMBOO_ACCOUNT "ralton66"  // your Temboo account name 
#define TEMBOO_APP_KEY_NAME "myFirstApp"  // your Temboo app key name
#define TEMBOO_APP_KEY  "fIjFsWhJaKDXmYBCRc9qCUvNI6FXVQvs"  // your Temboo app key

// The ID of the spreadsheet you want to send data to
// which can be found in the URL when viewing your spreadsheet at Google. For example, 
// the ID in the URL below is: "1tvFW2n-xFFJCE1q5j0HTetOsDhhgw7_998_K4sFtk"
// Sample URL: https://docs.google.com/spreadsheets/d/1tvFW2n-xFFJCE1q5j0HTetOsDhhgw7_998_K4sFtk/edit
#define SPREADSHEET_ID "16avg0X8H9OYpz6-nhuTzudoVBKYes8E3QtFxsiP8xaA"

//#if TEMBOO_LIBRARY_VERSION < 2
//#error "Your Temboo library is not up to date. You can update it using the Arduino library manager under Sketch > Include Library > Manage Libraries..."
//#endif

/* 
The same TembooAccount.h file settings can be used for all Temboo SDK sketches.  
Keeping your account information in a separate file means you can share the 
main .ino file without worrying that you forgot to delete your credentials.
*/


