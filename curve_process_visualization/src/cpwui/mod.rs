/* 
 * Juntong Liu
 *         2023.June.28
 * 
 * File name:    curve_process_visualization/src/cpwui/mod.rs
 * version 0.1
 * 
 * This program has been coded just for fine. It has not been tested.  Use it is at your own risk.
 *
 **/

use std::fs::File;
use std::io::{self, BufRead};
use std::path::Path;
use std::num::ParseFloatError;
use fltk::{prelude::*};
pub mod ui;
use crate::ui::{UserInterface};

#[derive(Debug)]
#[derive(Clone)]     
pub struct ProcessCurve {
    pub orig_bps: Vec<(f32, f32)>,
    delta_tanget: Vec<f32>,
    index_vec: Vec<(usize, f32)>,
    pub file_name: String,
    pub section_dvder: f32,
    pub divider_hasbeen_set: bool,
}


impl ProcessCurve {
    pub fn new() -> Self {      // constructor
        Self {
            orig_bps: Vec::new(),
            delta_tanget: Vec::new(),
            index_vec: Vec::new(),
            file_name: String::new(),
            section_dvder: 3.0,
            divider_hasbeen_set: false,
        }
    }

    fn process_curve_file(&mut self, ui: &mut UserInterface) -> Result<u32, i32> {
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
                    if elem.len() == 3 {       // with index
                        let x = elem[1].parse::<f32>().unwrap();
                        let y = elem[2].parse::<f32>().unwrap();
                        self.orig_bps.push((x, y));
                    }
                    else if elem.len() == 2 {  // without index
                        let x = convert_str_to_number(&elem[0]);
                        if x.is_err() {
                            panic!("Error convert x from string to number!");
                        }

                        let y = convert_str_to_number(&elem[1]);
                        if y.is_err() {
                            panic!("Error convert y from string to number!");
                        }
                        self.orig_bps.push((x.unwrap(), y.unwrap()));
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

        ui.frame_num_bps.set_label(&self.orig_bps.len().to_string());
        
        Ok(0)
    }

    fn validate_bps(&mut self) -> Result<u32, i32> {
        const TAKE_AWAY: f32 = -9999.9999;
        if self.orig_bps.iter().any(|pb| pb.0 < 0.0) {
            println!("Error! X must be greater or equal to 0");
            return Err(-1);
        }
        
        for n in 0..self.orig_bps.len() - 1 {
            if self.orig_bps[n].0 > self.orig_bps[n+1].0  {
                println!("Error, x is not in acending order!");
                return Err(-1);                
            }

            if self.orig_bps[n].0 == self.orig_bps[n+1].0 {
                println!("One bp will be taken away! Adjecent bps should not has same x");
                self.orig_bps[n+1].1 = TAKE_AWAY;
            }
        };

        self.orig_bps.retain(|&elem| elem.1 != TAKE_AWAY);

        Ok(0)
    }

    fn calculate_delta_tangent(&mut self) -> Result<u16, i16> {
        const FIRST: usize = 0;
        const SECOND: usize = 1;
        const THIRD: usize = 2;
        const CLOSED_SEC: f32 = 88888.0.
        let mut orig_index = 0;
        let orig_len = self.orig_bps.len();
        
        self.delta_tanget.clear();
        self.index_vec.clear();
        loop {
            let delta_x1 = self.orig_bps[orig_index + SECOND].0  - self.orig_bps[orig_index + FIRST].0;
            let delta_y1 = self.orig_bps[orig_index + SECOND].1  - self.orig_bps[orig_index + FIRST].1;

            let delta_x2 = self.orig_bps[orig_index + THIRD].0 - self.orig_bps[orig_index + SECOND].0;
            let delta_y2 = self.orig_bps[orig_index + THIRD].1 - self.orig_bps[orig_index + SECOND].1;
            
            let alfa: f32;
            if delta_x1 != 0_f32 && delta_x2 != 0_f32 {
                
                let alfa1 = delta_y1.atan2(delta_x1).to_degrees();
                let alfa2 = delta_y2.atan2(delta_x2).to_degrees();
                
                if (delta_y1 >= 0_f32 && delta_y2 >= 0_f32) || (delta_y1 <= 0_f32 && delta_y2 <= 0_f32) {   // both sections acending or decending
                    if alfa2 > alfa1 {
                        alfa = 180_f32 - (alfa2.abs() - alfa1.abs()).abs();
                        self.delta_tanget.push(alfa);
                    }
                    else if alfa1 >= alfa2 {
                        alfa = 90_f32 - alfa1.abs() + 90_f32 + alfa2.abs();
                        self.delta_tanget.push(alfa);
                    }
                }
                else if (delta_y1 >= 0_f32 && delta_y2 <= 0_f32) || (delta_y1 <= 0_f32 && delta_y2 >= 0_f32) {   // one acending, one decending
                        alfa = 180_f32 - (alfa1.abs() + alfa2.abs());
                        self.delta_tanget.push(alfa);
                }
            }
            else { 
                        let alfa = CLOSED_SEC;
                        self.delta_tanget.push(alfa);
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
        }

        for i in 0..self.delta_tanget.len() {
            let elem = (i, self.delta_tanget[i]);
            self.index_vec.push(elem);
        }
        
        self.delta_tanget.sort_by(|a, b| a.partial_cmp(b).unwrap());

        Ok(0)
    }

  
    pub fn process_bps(&mut self, num_bps_to_update: i32) -> Result<u32, i32> {
        const ADDITION: u32 = 1;
        const DEDUCTION: u32 = 0;
        const FIRST: usize = 0;
        const SECOND: usize = 1;
        const THIRD: usize = 2;
        let mut operation_flag: u32 = 3; 
        let orig_len = self.orig_bps.len();

        let num_bps = orig_len as i32 + num_bps_to_update;

        if (num_bps_to_update < 0) && (num_bps >= 2) {
                operation_flag = DEDUCTION;
        }
        else if num_bps_to_update > 0 {
                operation_flag = ADDITION;
        }
        else if num_bps < 2 {
                println!("{}Error! The minimum number of breakpoint one can have is 2  Try again!", '\n');
        }
      
        match operation_flag {
            DEDUCTION => {
                if self.calculate_delta_tangent() != Ok(0) {
                    println!("Error calculate delta-tangent!");
                    panic!("Error!");
                } 
                
                let tmplen = self.delta_tanget.len();
                if tmplen >= 1 {
                    let biggest_angle = self.delta_tanget[tmplen - 1];
                    let temp = self.index_vec.iter().find(|&elem| elem.1 == biggest_angle);
                    let (index_to_remove, _) = temp.expect("Error, try to find index for remove!");
                    self.orig_bps.remove(*index_to_remove + 1);
                }        
            },

            ADDITION => {
                if self.calculate_delta_tangent() != Ok(0) {
                    println!("Error calculate delta-tangent!");
                    panic!("Error!");
                } 
                    
                let sharp_angle = self.delta_tanget[0];
                let mut index_for_insert: usize = 0;
                for i in 0..self.index_vec.len() {
                    if sharp_angle == self.index_vec[i].1 { 
                        index_for_insert = self.index_vec[i].0;
                        if self.index_vec.len() > i + 1 {                   
                            if sharp_angle == self.index_vec[i + 1].1 {
                                continue;
                            }
                            else {
                                break;
                            }
                        }
                        else {
                            break;
                        }
                    }
                    else {
                        continue;
                    }
                }


                let ratio: f32;
                let delta_x1 = self.orig_bps[index_for_insert + SECOND].0 - self.orig_bps[index_for_insert + FIRST].0;
                let delta_x2 = self.orig_bps[index_for_insert + THIRD].0 - self.orig_bps[index_for_insert + SECOND].0;
                let delta_y1 = self.orig_bps[index_for_insert + SECOND].1 - self.orig_bps[index_for_insert + FIRST].1;
                let delta_y2 = self.orig_bps[index_for_insert + THIRD].1 - self.orig_bps[index_for_insert + SECOND].1;
            
                if ((delta_x1 * delta_x1) + (delta_y1 * delta_y1)).sqrt() <  ((delta_x2 * delta_x2) + (delta_y2 * delta_y2)).sqrt() {
                        ratio = ((delta_x1 * delta_x1) + (delta_y1 * delta_y1)).sqrt() / ((delta_x2 * delta_x2) + (delta_y2 * delta_y2)).sqrt();
                }
                else if ((delta_x1 * delta_x1) + (delta_y1 * delta_y1)).sqrt() <  ((delta_x2 * delta_x2) + (delta_y2 * delta_y2)).sqrt() {
                        ratio = ((delta_x2 * delta_x2) + (delta_y2 * delta_y2)).sqrt() / ((delta_x1 * delta_x1) + (delta_y1 * delta_y1)).sqrt();             
                }
                else {
                        ratio = 1.0;
                }

                let x1 = self.orig_bps[index_for_insert + FIRST].0      
                            + (self.orig_bps[index_for_insert + SECOND].0 - self.orig_bps[index_for_insert + FIRST].0) * (1.0 - 1.0/self.section_dvder);
                let y1 = self.orig_bps[index_for_insert + FIRST].1
                            + (self.orig_bps[index_for_insert + SECOND].1 - self.orig_bps[index_for_insert + FIRST].1) * (1.0 - 1.0/self.section_dvder);

                let x2 = self.orig_bps[index_for_insert + SECOND].0 
                            + (self.orig_bps[index_for_insert + THIRD].0 - self.orig_bps[index_for_insert + SECOND].0) * ratio * 1.0/self.section_dvder;
                let y2 = self.orig_bps[index_for_insert + SECOND].1
                            + (self.orig_bps[index_for_insert + THIRD].1 - self.orig_bps[index_for_insert + SECOND].1) * ratio * 1.0/self.section_dvder;
        
                self.orig_bps.insert(index_for_insert + 1, (x1, y1)); 
                self.orig_bps.insert(index_for_insert + 3, (x2, y2)); 
                self.orig_bps.remove(index_for_insert + 2);
            },
            _ => (),
        }

        Ok(0)
    }

    pub fn print_result(&mut self) {
        println!("{}{} breakpoints have been picked. They are: {}", '\n', self.orig_bps.len(), '\n'); 
        for i in 0..self.orig_bps.len(){
            println!("{}.)   {}  {} ", i, self.orig_bps[i].0, self.orig_bps[i].1);  
        }
        println!("");

    }

    pub fn get_max_x(&self) -> f32 {
       if self.orig_bps.len() > 3 {
            let mut max_x: f32 = 0.0;
            for elem in &self.orig_bps {
                if elem.0 > max_x {
                    max_x = elem.0;
                }
            }
            max_x
        }
        else {
            10.0 // we return a 10 to keep the coord open
        }
    }
    
    pub fn get_max_y(&self) -> f32 {
       if self.orig_bps.len() > 3 {
            let mut max_y: f32 = 0.0;
            for elem in &self.orig_bps {
                if elem.1 > max_y {
                    max_y = elem.1;
                }
            }
            max_y
        }
        else {
            10.0
        }
    }

    pub fn set_divider(&mut self, dvder: f32) {     
            self.section_dvder = dvder;
    }

    pub fn get_divider_flag(&mut self) -> bool {
        return self.divider_hasbeen_set;
    }
    
    pub fn set_divider_flag(&mut self) {
        self.divider_hasbeen_set = true;
    }
}

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

pub fn run(pobj: &mut ProcessCurve, ui: &mut UserInterface) -> Result<u32, i32> {
    let mut file_name = String::new();
    loop {
        file_name.clear();
        println!("{}Type a curve file to process, or hit Enter to use a default test curve file.", '\n');
        println!("Type q or Q to quit!");
        std::io::stdin()
            .read_line(&mut file_name)
            .expect("Failed to read in file name!");
        file_name = file_name.trim().to_string();
        if file_name == "q" || file_name == "Q" {
            return Err(-1);
        }

        if file_name.len() == 0 {
            file_name = String::from("./test.curve");
        }

        println!("You have typed in: {}.", file_name);
        // check to see if the file exist
        let file_check = std::path::Path::new(&file_name).exists();
        if file_check != true {
            println!("{}File {} does not exist! Try again!", '\n', file_name);
            continue;
        }
        else {
            pobj.file_name = file_name.to_string();
            break;
        }
    }

    if pobj.process_curve_file(ui) != Ok(0) {
        panic!("File process error!");
    }

    if pobj.validate_bps() != Ok(0) {
        panic!("Breakpoint validation failed!");
    }

    pobj.print_result();
    Ok(0)
}
