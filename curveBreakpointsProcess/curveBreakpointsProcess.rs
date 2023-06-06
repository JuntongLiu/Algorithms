/*
 * Juntong Liu
 *           2023.June.03
 * 
 * curve_breakpoint_process.rs    (Rust implementation)
 * 
 * This program process a curve breakpoints file. According to user's request, the program will merge those adjacent 
 * curve sections that have smallest slope devication from each other, so that the total number of breakpoints describing 
 * the curve can be reduced to user requested level.
 * The purpose to do this is because that some curve files might contain a large number of breakpoints, but the maximum 
 * number of breakpoints that a device can have is limited.
 * However, please note that for a no lieaner??? curve, more breakpoints there are, the better the description will be. 
 * So, do not reduce curve breakpoints if it is not necessary. 
 * 
 * Each line of the curve file contain 2 float point number separated by whitespace or tab, something like:
 * 
 *  1.  1.111  100.111
 *      2.222  220.222
 *  3.  3.333  300.333
 *      .....
 * 
 * There can be a sequence number at the begining of each line.
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
    pub fn new(x: f32, y: f32) -> Self {     // constructor
        Self {
            sensor_unit: x,
            temp: y,
        }
    }
}

struct ProcessCurve {
    orig_bps: Vec<BreakPoint>,
    delta_tanget: Vec<f32>,
    file_name: String,
}

impl ProcessCurve {
    fn new() -> Self {      // constructor
        Self {
            orig_bps: Vec::new(),
            delta_tanget: Vec::new(),
            file_name: String::new(),
        }
    }

    fn process_curve_file(&mut self) -> Result<u32, i32> {
        if let Ok(lines) = read_lines(&self.file_name) {
            for line in lines {
                // Here we need to skip the comments line begining with #
                //let bytess = line.as_ref().unwrap().bytes();    // add .as_ref() or .as_mut() to bowrrow the lines
                
                if let Ok(pair) = line {
                    let pair = pair.trim();
                    let sp_elem: Vec<&str> = pair.split(&[' ', '\t'][..]).collect::<Vec<_>>();
                    // Make it more tolerant
                    let elem = sp_elem.into_iter().filter(|item| item.len() > 1).collect::<Vec<&str>>();
                    if elem.len() == 3 {       // with index number
                        let x = elem[2].parse::<f32>().unwrap();
                        let y = elem[3].parse::<f32>().unwrap();
                        let newbp = BreakPoint::new(x, y);
                        self.orig_bps.push(newbp);
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

                        let newbp = BreakPoint::new(x.unwrap(), y.unwrap());
                        self.orig_bps.push(newbp);
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

    // Make sure that the x values are in acending order.
    // return -1 = Error;  0 = Passed OK
    fn validate_bps(&self) -> Result<u32, i32> {
        // make sure that all x are >= 0
        if self.orig_bps.iter().any(|pb| pb.sensor_unit < 0.0) {
            println!("Error! X must be greater or equal to 0");
            return Err(-1);
        }
        
        for n in 0..self.orig_bps.len() - 1 {
            if self.orig_bps[n].sensor_unit > self.orig_bps[n+1].sensor_unit  {
                println!("Error, x is not in acending order!");
                return Err(-1);                
            }
        };

        Ok(0)
    }

    fn process_bps(&mut self) -> Result<u32, i32> { 
        const FIRST: usize = 0;
        const SECOND: usize = 1;
        const THIRD: usize = 2;
        const TAKE_AWAY: f32 = -999.999;
        let mut orig_index = 0;
        let orig_len = self.orig_bps.len();
        let mut user_input = String::new();
        let mut num_bps_user_want;
        let mut tuple_vec: Vec<(usize, f32)> = Vec::new();
        
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

            if self.orig_bps.len() <= num_bps_user_want {
                println!("The number of break points is less or equal to you required, no need to reduce anymore!");
                println!("Try again!");
                continue 'try_again;
            }
            
            if num_bps_user_want < 2 {
                println!("{}Error! The minimum number of breakpoint one can have is 2  Try again!", '\n');
                continue 'try_again;
            }

            break;
        }

        loop {
            let delta_x = self.orig_bps[orig_index + SECOND].temp - self.orig_bps[orig_index + FIRST].temp;
            let delta_y = self.orig_bps[orig_index + SECOND].sensor_unit - self.orig_bps[orig_index + FIRST].sensor_unit;
            let tang1 =  delta_x / delta_y;

            let delta_x = self.orig_bps[orig_index + THIRD].temp - self.orig_bps[orig_index + SECOND].temp;
            let delta_y = self.orig_bps[orig_index + THIRD].sensor_unit - self.orig_bps[orig_index + SECOND].sensor_unit;
            let tang2 = delta_x / delta_y;
            
            if (tang1 > 0.0 && tang2 > 0.0) || (tang1 < 0.0 && tang2 < 0.0) { // both section acending or decending

                let delta_t = (tang1 - tang2).abs();
                self.delta_tanget.push(delta_t);      // save in vector

            }
            else {      // one section acending and another section decending
                let delta_t = tang1.abs() + tang2.abs();
                self.delta_tanget.push(delta_t);      // save in vector            

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

        } // end of loop

        // record the tanget in a vector of tuple
        for i in 0..self.delta_tanget.len() {
            let elem = (i, self.delta_tanget[i]);
            tuple_vec.push(elem);
        }

        // now we sort the delta_tanget vector
        self.delta_tanget.sort_by(|a,b| a.partial_cmp(b).unwrap());

        // calculate how many bps need to be taken away
        let take_away = orig_len - num_bps_user_want;
        
        for i in 0..take_away {
            let delta_t = self.delta_tanget[i]; 
            //let temp = tuple_vec.iter().find(|&elem| elem.1 == delta_t);
            for n in 0..tuple_vec.len() {    // go through all elem in case there are duplicated items 
                if tuple_vec[n].1 == delta_t {
                    let (del_orig_index, _) = tuple_vec[n];
                    self.orig_bps[del_orig_index + 1].sensor_unit = TAKE_AWAY;
                    self.orig_bps[del_orig_index + 1].temp = TAKE_AWAY;
                }
            }
        } 

        self.orig_bps.retain(|&elem| elem.sensor_unit != TAKE_AWAY);
        Ok(0)
    }

    fn print_result(&self) {
        println!("{}{} breakpoints have been picked. They are: {}", '\n', self.orig_bps.len(), '\n'); 
        for elem in &self.orig_bps {
            println!(" {}  {} ", elem.sensor_unit, elem.temp);        
        }
        println!("");
    }
}

// The output is wrapped in a Result to allow matching on errors
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
