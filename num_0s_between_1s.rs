/*
 * Juntong Liu
 *            2023.03.07: 18:12
 *  
 * File name: num_0s_between_1s.rs
 *
 * This program takes an integer from user and counts those continuous 0 bits between two 1 bits in the integer. 
 * Those lengthes of continuous 0 bits between two 1 bits will be saved in an arry and the longest length will be printed out.
 * 
 * For example, on a machine where integer_size = 32 bits, 
 * 
 *  5678 = 00000000000000000001011000101110      The longest length of continuous 0 bits between two 1 bits is: 3
 *                                ^^^
 *
 *   123 = 00000000000000000000000001111011      The longest length of continuous 0 bits between two 1 bits is: 1
 *                                      ^
 * 
 *  -123 = 11111111111111111111111110000101      The longest length of continuous 0 bits between two 1 bits is: 4
 *                                  ^^^^
 * To compile and run the program:
 *       
 *  $ rustc num_0s_between_1s.rs
 *  $ ./num_0s_between_1s
 *
 */



const RSL_LEN: usize = 16;   

fn num_z(n: i32) -> usize {

    let mut oneb: i32 = 1;
    let mut count: usize = 0; 
    let mut resultindex = 0;
    let mut checkedb = 0;
    const INT_SIZE: i32 = 32;
    let mut result:[usize; RSL_LEN] = [0; RSL_LEN];
    let integer: i32 = n;

    while checkedb < INT_SIZE {  
        if oneb & integer == 0 {
            oneb <<= 1;
            checkedb += 1;
        }
        else {
            break;
        }
    }

    while checkedb < INT_SIZE {          
        while checkedb < INT_SIZE {
            if oneb & integer != 0 {      
                checkedb += 1;
                oneb <<= 1;
            }
            else {
                break;
            }
        }
        while checkedb < INT_SIZE {
            if oneb & integer == 0 {
                count += 1;
                checkedb += 1;
                oneb <<= 1;
            }
            else { 
                  result[resultindex] = count;
                  resultindex += 1;
                  count = 0;
                break;
            }
        }
    }
    
    if resultindex == 0 {
        return 0; 
    }
    else {
            sort_array(&mut result);
            find_max_element(&mut result);
            result[RSL_LEN - 1]
       }
}

/*  Use array for now, we can use vector later */
fn sort_array(int_arr: &mut [usize; RSL_LEN]){
    let len = RSL_LEN - 1;
    let mut i: usize = 0;
    
    //println!("Array before sorting: {:?}", int_arr);
    while i <= len
    {
        let mut min = i;
        let mut j = i + 1;
        while j <= len
        {
            if int_arr[j] < int_arr[min] {
                min = j;
            } 
            j += 1;
        }
        let temp = int_arr[i];
        int_arr[i] = int_arr[min];
        int_arr[min] = temp;
        i += 1;
    }
    //println!("Array after sorting: {:?}", int_arr);

}

fn find_max_element(int_arr: &mut [usize; RSL_LEN])
{
    let mut i: usize = 0;
    let mut large = int_arr[0];
    while i < int_arr.len()
    {
        if large < int_arr[i].try_into().unwrap()
        {
            large = int_arr[i];
        }
        i += 1;
    }
    println!("The longest element in the array is: {}", large);
}

fn main() 
{
    let mut user_in = String::new();
    
    loop{
        user_in.clear();
        println!("Please enter an integer number or enter q or Q to quit the program:");
        std::io::stdin()
            .read_line(&mut user_in)
            .expect("Failed to read in line!");
        
        let user_in = user_in.trim();
        if user_in == "q" || user_in == "Q" {
            return; 
        }

        let a_integer: i32 = match user_in.trim().parse()
        {
            Ok(num) => num, 
            Err(_)  => {
                println!("Your input is not a number, try again!");
                continue;
            }
        };
        println!("You have entered: {:b}", a_integer);
        let max_0s: i32 = num_z(a_integer).try_into().unwrap();
        println!("The maximum length of continous 0s between two 1s is: {}", max_0s);
    }
}
