def compile_pattern(FSM, pattern):
    for c in pattern:
        col = [0] * 128;
        col[ord(c)] = len(FSM) + 1;
        FSM.append(col);

def compile_non_deterministic_pattern(FSM, pattern):
    k = len(FSM) + len(pattern);
    for c in pattern:
        col = [0] * 128;
        col[ord(c)] = k;
        col[127] = len(pattern);
        FSM.append(col);

def match_string(FSM, string):
    state = 0;
    char_index = 0;
    match_length = 1;

    while char_index < len(string):
        old_state = state;
        state = FSM[state][ord(string[char_index])];

        #print(string[char_index], state, old_state, FSM[old_state][127])

        if old_state + 1 >= len(FSM):
            break;

        if state == 0:
            it = FSM[old_state + 1][127] - 1;
            for _ in range(it):
                if state == 0:
                    old_state += 1;
                    state = FSM[old_state][ord(string[char_index])];
                    #print(string[char_index], state, old_state, FSM[old_state][127])

            if state == 0:
                return False, match_length;


        if state == len(FSM):
            break;

        match_length += 1;
        char_index += 1;

    return state == len(FSM), match_length;

def locate_matches(FSMs, string):
    matches_loc = [];
    i = 0;
    while i < len(string):
        is_match = False;
        for FSM in FSMs:
            is_match, match_len = match_string(FSM, string[i:]);
            #NOTE: race condition? what if the string matches two patterns, the first one wins i guess, for my use case it's not a problem, for a general case i might check this further
            #print(string[i:], str(is_match), match_len);
            if is_match:
                matches_loc.append((i, match_len));
                i += match_len;   
                break;

        if not is_match:
                i += 1;

    return matches_loc;

def print_FSM(FSM):
    for col in FSM:
        for c in col:
            print(c, end='');
        print('');

ALPHA = "ABCDEFGHIJKLMNOPQRSTUVWXYZ+";
NUMERIC = "0123456789";
BIN_OP = "+-/*:()";

#FSM1 = [];

#compile_non_deterministic_one_or_more_pattern(FSM1, ALPHA);

FSM1 = [];
compile_non_deterministic_pattern(FSM1, ALPHA);
compile_non_deterministic_pattern(FSM1, NUMERIC);
compile_non_deterministic_pattern(FSM1, NUMERIC);

FSM2 = [];
compile_non_deterministic_pattern(FSM2, BIN_OP);

FSM3 = [];
compile_non_deterministic_pattern(FSM3, NUMERIC);

FSM4 = [];
compile_pattern(FSM4, "MIN");

FSMs = [FSM1, FSM2, FSM3, FSM4];
strings = [" 13 + 2 -MIN(A12:B15)"];
for string in strings:
    print(string);
    matches = locate_matches(FSMs, string);
    for match_index, match_length in matches:
        print("  " + string[match_index:match_index+match_length]);
