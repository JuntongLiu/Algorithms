/*
 * Juntong Liu
 *           2023.June.03
 * 
 * curve_breakpoint_process.rs    (Rust implementation)
 * 
 * This program read breakpoints from a curve file and do following process according to user's request.
 * 
 * The program will act according to requested number received from user:
 * 
 *    1.) Add more breakpoints if user type in a number that is bigger than the number of breakpoints in the curve file.
 *        New breakpoints will be added to sections with biggest slop-deviation to smooth the curve graph.
 *    2.) Take away breakpoints from the sections that have the smallest slop-deviation so that the totall number of 
 *        breakpoints can be reduced to a required number.
 *  
 * Each line of the curve file contain 2 float point number separated by whitespace or tab, something like:
 * There can be a sequence number at the begining of each line.
 * 
 * Note, this program has been coded just for fine. It has not been tested and use it is at your own risk!
 * 
 * To compile and run:
 *
 *      $ rustc  curve_breakpoint_process.rs
 *      $ ./curve_breakpoint_process
 *      (following the prompt)
 */

use std::fs::File;
use std::io::{self, BufRead};
use std::path::Path;
use std::num::ParseFloatError;

#[derive(Debug)]
#[derive(Copy, Clone)]     
struct BreakPoint
{
    sensor_unit: f32,
    temp: f32,
}

impl BreakPoint {
    pub fn new(x: f32, y: f32) -> Self {   // constructor
        Self {
            sensor_unit: x,
            temp: y,
        }
    }
}

struct ProcessCurve {
    orig_bps: Vec<BreakPoint>,
    delta_tanget: Vec<f32>,
    tuple_vec: Vec<(usize, f32)>,
    file_name: String,
}

impl ProcessCurve {
    fn new() -> Self {      // constructor
        Self {
            orig_bps: Vec::new(),
            delta_tanget: Vec::new(),
            tuple_vec: Vec::new(),
            file_name: String::new(),
        }
    }

    fn process_curve_file(&mut self) -> Result<u32, i32> {
        if let Ok(lines) = read_lines(&self.file_name) {
            for line in lines {
                if let Ok(pair) = line {
                    let pair = pair.trim();
                    let sp_elem: Vec<&str> = pair.split(&[' ', '\t'][..])
                                                .collect::<Vec<_>>();
                    // Make it more tolerant
                    let elem = sp_elem
                                    .into_iter()
                                    .filter(|item| item.len() > 1)
                                    .collect::<Vec<&str>>();
                    if elem.len() == 3 {       // with index number
                        let x = elem[1].parse::<f32>().unwrap();
                        let y = elem[2].parse::<f32>().unwrap();
                        self.orig_bps.push(BreakPoint::new(x, y));
                    }
                    else if elem.len() == 2 {  // without index number
                        let x = convert_str_to_number(&elem[0]);
                        if x.is_err() {
                            panic!("Error convert x from string to number!");
                        }

                        let y = convert_str_to_number(&elem[1]);
                        if y.is_err() {
                            panic!("Error convert y from string to number!");
                        }
                        self.orig_bps.push(BreakPoint::new(x.unwrap(), y.unwrap()));
                    }
                    else if elem.len() == 0 {
                        continue;
                    }
                    else {
                        println!("File format error!");
                        return Err(-1);
                    }
                }
                else{
                    println!("File line error!");
                    return Err(-1);
                }
            }
        }
        else {
            println!("File Reading Error!");
            return Err(-1);
        }

        if self.orig_bps.len() < 3 {
            println!("File must contain more than 3 BPs.");
            return Err(-1);
        }

        Ok(0)
    }

    // Make sure that the curve file does not contain error.
    fn validate_bps(&mut self) -> Result<u32, i32> {
        const TAKE_AWAY: f32 = -9999.9999;
        // make sure that all x are >= 0
        if self.orig_bps.iter().any(|pb| pb.sensor_unit < 0.0) {
            println!("Error! x must be greater or equal to 0");
            return Err(-1);
        }
        
        for n in 0..self.orig_bps.len() - 1 {
            if self.orig_bps[n].sensor_unit > self.orig_bps[n+1].sensor_unit  {
                println!("Error, x is not in acending order!");
                return Err(-1);                
            }

            if self.orig_bps[n].sensor_unit == self.orig_bps[n+1].sensor_unit {
                println!("One bp will be taken away! Adjecent bps should not has same x");
                self.orig_bps[n+1].temp = TAKE_AWAY;
            }
        };

        self.orig_bps.retain(|&elem| elem.temp != TAKE_AWAY);

        Ok(0)
    }

    fn calculate_delta_tangent(&mut self) -> Result<u16, i16> {
        const FIRST: usize = 0;
        const SECOND: usize = 1;
        const THIRD: usize = 2;
        let mut orig_index = 0;
        let orig_len = self.orig_bps.len();
        
        self.delta_tanget.clear();
        self.tuple_vec.clear();
        loop {
            let delta_x = self.orig_bps[orig_index + SECOND].sensor_unit - self.orig_bps[orig_index + FIRST].sensor_unit;
            let delta_y = self.orig_bps[orig_index + SECOND].temp - self.orig_bps[orig_index + FIRST].temp;
            let tang1 =  delta_y / delta_x;

            let delta_x = self.orig_bps[orig_index + THIRD].sensor_unit - self.orig_bps[orig_index + SECOND].sensor_unit;
            let delta_y = self.orig_bps[orig_index + THIRD].temp - self.orig_bps[orig_index + SECOND].temp;
            let tang2 = delta_y / delta_x;
            
            if (tang1 > 0.0 && tang2 > 0.0) || (tang1 < 0.0 && tang2 < 0.0) {

                let delta_t = (tang1 - tang2).abs();
                self.delta_tanget.push(delta_t);

            }
            else {
                let delta_t = tang1.abs() + tang2.abs();
                self.delta_tanget.push(delta_t);          
            }
            if orig_index + 1 <= orig_len - 1 {
                orig_index += 1;
            }
            if orig_index + 2 <= orig_len - 1 {
                continue;
            }
            else {
                break;
            }

        }    // end of loop

        for i in 0..self.delta_tanget.len() {
            let elem = (i, self.delta_tanget[i]);
            self.tuple_vec.push(elem);
        }

        self.delta_tanget.sort_by(|a, b| a.partial_cmp(b).unwrap());
        Ok(0)
    }

    fn process_bps(&mut self) -> Result<u32, i32> { 
        const TAKE_AWAY: f32 = -9999.9999;
        const ADDITION: u32 = 1;
        const DEDUCTION: u32 = 0;
        let operation_flag: u32; 
        let mut num_bps_to_add: usize = 0;
       
        let orig_len = self.orig_bps.len();
        let mut user_input = String::new();
        let mut num_bps_user_want;

        'try_again: loop {
            user_input.clear();
            println!("{}The file contains {} break points", '\n', self.orig_bps.len());
            println!("How many break point you want to have/keep?{}", '\n');
            std::io::stdin()
                .read_line(&mut user_input)
                .expect("Failed to read input from user!");
        
            num_bps_user_want = match user_input.trim().parse() {
                Ok(num) => num,
                Err(_) => {
                    println!("Your input is not a number, try again!");
                    continue 'try_again;
                }
            };

            if self.orig_bps.len() == num_bps_user_want {
                println!("The number of break points is equal to you required, no need to do anything!");
                println!("Try again!");
                continue 'try_again;
            }

            if (self.orig_bps.len() > num_bps_user_want) && (num_bps_user_want >= 2) {
                operation_flag = DEDUCTION;
                break;
            }  
            else if self.orig_bps.len() < num_bps_user_want {
                operation_flag = ADDITION;
                num_bps_to_add = num_bps_user_want - self.orig_bps.len() + (num_bps_user_want - self.orig_bps.len())%2;
                println!("{} breakpoints will be added to smooth the curve.", num_bps_to_add);
                break;
            }
            else if num_bps_user_want < 2 {
                println!("{}Error! The minimum number of breakpoint one can have is 2  Try again!", '\n');
                continue 'try_again;
            }
        
        }

        match operation_flag {
            DEDUCTION => {
                if self.calculate_delta_tangent() != Ok(0) {
                    println!("Error calculate delta-tangent!");
                    panic!("Error!");
                } 
                let take_away = orig_len - num_bps_user_want;
                for i in 0..take_away {
                    let delta_t = self.delta_tanget[i]; 
                    for n in 0..self.tuple_vec.len() {
                        if self.tuple_vec[n].1 == delta_t {
                            let (del_orig_index, _) = self.tuple_vec[n];
                            self.orig_bps[del_orig_index + 1].sensor_unit = TAKE_AWAY;
                            self.orig_bps[del_orig_index + 1].temp = TAKE_AWAY;
                        }
                    }
                } 
                self.orig_bps.retain(|&elem| elem.sensor_unit != TAKE_AWAY);
            },

            ADDITION => {
                while num_bps_to_add != 0 {
                    if self.calculate_delta_tangent() != Ok(0) {
                        println!("Error calculate delta-tangent!");
                        panic!("Error!");
                    } 

                    let index = self.delta_tanget.len() - 1;
                    let bigest_delta = self.delta_tanget[index];
                    let temp = self.tuple_vec.iter().find(|&elem| elem.1 == bigest_delta);
                    let (index_for_insert, _) = temp.expect("Error, try to find index!");

                    let x1 = self.orig_bps[*index_for_insert].sensor_unit 
                            + (self.orig_bps[*index_for_insert + 1].sensor_unit - self.orig_bps[*index_for_insert].sensor_unit)/2.0;
                    let y1 = self.orig_bps[*index_for_insert].temp 
                            + (self.orig_bps[*index_for_insert + 1].temp - self.orig_bps[*index_for_insert].temp)/2.0;
                    let x2 = self.orig_bps[*index_for_insert + 1].sensor_unit 
                            + (self.orig_bps[*index_for_insert + 2].sensor_unit - self.orig_bps[*index_for_insert + 1].sensor_unit)/2.0;
                    let y2 = self.orig_bps[index_for_insert + 1].temp
                            + (self.orig_bps[*index_for_insert + 2].temp - self.orig_bps[*index_for_insert + 1].temp)/2.0;

                    self.orig_bps.insert(index_for_insert + 1, BreakPoint::new(x1, y1));
                    self.orig_bps.insert(index_for_insert + 3, BreakPoint::new(x2, y2));
                    num_bps_to_add -= 2; 
                
                }
            },
            _ => (),
        }

        Ok(0)
    }

    // print the result bps
    fn print_result(&self) {
        println!("{}{} breakpoints have been picked. They are: {}", '\n', self.orig_bps.len(), '\n'); 
        for i in 0..self.orig_bps.len(){
            println!("{}.)   {}  {} ", i, self.orig_bps[i].sensor_unit, self.orig_bps[i].temp);  
        }
        println!("");
    }
}

// Returns an Iterator to the Reader of the lines of the file.
fn read_lines<P>(filename: P) -> io::Result<io::Lines<io::BufReader<File>>>
where P: AsRef<Path>, {
    let file = File::open(filename)?;
    Ok(io::BufReader::new(file).lines())
}

fn convert_str_to_number(number_str: &str) -> Result<f32, ParseFloatError> {
    let number = match number_str.trim().parse::<f32>() {
        Ok(number) => number,
        Err(e) => return Err(e),
    };
    Ok(number)
}

fn run(pobj: &mut ProcessCurve) {
    let mut file_name = String::new();
    loop {
        file_name.clear();
        println!("{}Type a curve file name to process, or q/Q to quit:", '\n');
        std::io::stdin()
            .read_line(&mut file_name)
            .expect("Failed to read in file name!");
        let file_name = file_name.trim();
        if file_name == "q" || file_name == "Q" {
            return;
        }
        println!("You have typed in: {}.", file_name);
        // check to see if the file exist
        let file_check = std::path::Path::new(file_name).exists();
        if file_check != true {
            println!("{}File {} does not exist! Try again!", '\n', file_name);
            continue;
        }
        else {
            pobj.file_name = file_name.to_string();
            break;
        }
    }

    if pobj.process_curve_file() != Ok(0) {
        panic!("File process error!");
    }

    if pobj.validate_bps() != Ok(0) {
        panic!("Breakpoint validation failed!");
    }

    if pobj.process_bps()  != Ok(0) {
        panic!("Error in processing bps!");
    }

    pobj.print_result();
}

fn main() {

    let mut process_curve = ProcessCurve::new();     // create a processCurve obj
    run(&mut process_curve);

}
