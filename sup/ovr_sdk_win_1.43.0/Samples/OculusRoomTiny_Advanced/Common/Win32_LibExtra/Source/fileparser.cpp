
#include "../Header/libextra.h"

/// The rules of the data files
/// ===========================
/// 1.  End file with the word END - upper case!!
/// 2.  Each keyword and its arguments should not have any spaces or \n
/// 3.  Arguments must be surrounded by () and be separated by ,
/// 4.  If no arguments, keywords can be left on their own.

// Access Functions
//-----------------------------------------------------------------
int FileParser::Int(char * name, int argumentNum)    { return(GetKeyWord(name)->argument[argumentNum]->data_as_int); }
float FileParser::Float(char * name,int argumentNum)    {    return(GetKeyWord(name)->argument[argumentNum]->data_as_float);}
char * FileParser::String(char * name,int argumentNum)    {    return(GetKeyWord(name)->argument[argumentNum]->data_as_string);}

//-----------------------------------------------------------------
FileParser::FileParser(int arg_max_keywords, int arg_max_supply_arguments)
{
    // Setup and memory
    max_keywords = arg_max_keywords;
    num_keywords=0;
    keyword = (KEYWORD *) malloc(max_keywords*sizeof(KEYWORD));

    max_supply_arguments = arg_max_supply_arguments;
    num_supply_arguments = 0;
    supply_argument = (ARGUMENT *) malloc(max_supply_arguments*sizeof(ARGUMENT));
}

//-----------------------------------------------------------------
FileParser::KEYWORD * FileParser::GetKeyWord(char * name)
{
    for (int i = 0; i<num_keywords; i++)
    {
        if (!strcmp(name, keyword[i].keyword)) return(&keyword[i]);
    }
    Debug::Error("Error in config file. There is a problem with '%s'.\n", name);
    return NULL;
}

//-----------------------------------------------------------------
int Return_Location_Of_Special_Character_Or_Minus1(char * string, char special_character)
{
    int location = 0;
    while(string[location]!='\0')
    {
        if (string[location]==special_character) return(location);
        location++;
    }
    return(-1);
}

//-----------------------------------------------------------------
void Curtail_String_Before(char * string,char special_character)
{
    int location = Return_Location_Of_Special_Character_Or_Minus1(string,special_character);
    if (location != -1)    string[location] = '\0';
}

//-----------------------------------------------------------------
void Delete_String_Until_And_Including(char * string, char special_character)
{
    int location = Return_Location_Of_Special_Character_Or_Minus1(string,special_character);
    if (location != -1)
    {
        // We shunt up everything after location, to the start, including end \0
        int length = (int) strlen(string);
        for (int i=location+1;i<=length;i++)
            string[i-location-1] = string[i];
    }
    else
    {
        // Delete the whole thing if character not found
        string[0] = '\0';
    }
}

//-----------------------------------------------------------------
void Write_Front_String_Before_Symbol_Then_Delete_It_And_Symbol(char * main_string, char symbol, char * front_string_of_same_size)
{
    strcpy_s(front_string_of_same_size,1+strlen(main_string),main_string);
    Curtail_String_Before(front_string_of_same_size,symbol);
    Delete_String_Until_And_Including(main_string,symbol);
}

//-----------------------------------------------------------------
void FileParser::Fill_From_File(char * filename)
{
    // Reset things
    num_keywords=0;
    num_supply_arguments=0;

    // Now lets look through the file itself
    FILE * fileptr;  fopen_s(&fileptr, File.Path(filename,true), "r");

    static char whole_string[MAX_CHARS_IN_KEYWORD_AND_ARGUMENTS];
    static char front_string[MAX_CHARS_IN_KEYWORD_AND_ARGUMENTS];  

    while(1)
    {
        // Read in a string
        fscanf_s(fileptr,"%s",whole_string,MAX_CHARS_IN_KEYWORD_AND_ARGUMENTS);

        // If its the end, then lets terminate
        if (!strcmp(whole_string,"END"))
        {
            fclose(fileptr);
            return;
        }

        // Prepare structure to write results into
        KEYWORD * kw = &keyword[num_keywords];
        kw->num_arguments=0;
        num_keywords++;
        Debug::Assert(num_keywords < max_keywords,"Too many keywords");

        // Get the keyword
        Write_Front_String_Before_Symbol_Then_Delete_It_And_Symbol(whole_string,'(',front_string);
        strcpy_s(kw->keyword,1+strlen(front_string),front_string); // Write in keyword into structure
        Curtail_String_Before(whole_string,')');  // Lets take off any final bracket, leaving just the arguments separated by commas

        // Loop through getting the arguments
        while(strlen(whole_string))  // Stops if nothing more to look at
            {
            Write_Front_String_Before_Symbol_Then_Delete_It_And_Symbol(whole_string,',',front_string);
            ARGUMENT * a = &supply_argument[num_supply_arguments];
            num_supply_arguments++;  Debug::Assert(num_supply_arguments < max_supply_arguments,"Not enough supply arguments");
            kw->argument[kw->num_arguments] = a;
            kw->num_arguments++; Debug::Assert(kw->num_arguments < MAX_ARGUMENTS_PER_KEYWORD,"Too many arguments per keyword");
            // Write argument into structure
            strcpy_s(a->data_as_string,1+strlen(front_string),front_string); 
            double d = atof(front_string);
            float f = (float) d;
            int i = (int) f;
            a->data_as_float = f;
            a->data_as_int = i;
        }
    }
    fclose(fileptr);
}

//---------------------------------------------------------------
void FileParser::Debug_Output(void)
{
    Debug::Output("Number of keywords = %d\n",num_keywords);
    for (int i=0;i<num_keywords;i++)
    {
        KEYWORD * kw = &keyword[i];
        Debug::Output("KEYWORD=%s NumArgs=%d ",kw->keyword,kw->num_arguments);
        for (int j=0;j<kw->num_arguments;j++)
        {
            ARGUMENT * a = kw->argument[j];
            Debug::Output("arg%d=(%s,%0.3f,%d) ",j,a->data_as_string,a->data_as_float,a->data_as_int);
        }
        Debug::Output("\n");
    }
}



