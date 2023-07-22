/* 
 * Juntong Liu
 *         2023.June.28
 * 
 * File name: curve_process_visualization/src/main.rs   
 * version 0.11
 * 
 * This program has been coded just for fine. It has not been tested.  Use it is at your own risk.
 *
 **/

use fltk::{prelude::*, *};
use plotters::prelude::*;
mod cpwui;
use cpwui::*;


#[derive(Debug, Clone, Copy)]
pub enum Message {
    Increment,
    Decrement,
    Choice1Msg,
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut pcbp = ProcessCurve::new(); 
    let app = app::App::default();
    let mut ui = ui::UserInterface::make_window();
    
    if cpwui::run(&mut pcbp, &mut ui) == Err(-1) {
        panic!("Error running program!");
    }    

    let x_max = pcbp.get_max_x();
    let y_max = pcbp.get_max_y();
  
    let (s, r) = app::channel::<Message>();
    ui.plus_but.emit(s, Message::Increment);
    ui.minus_but.emit(s, Message::Decrement);
    ui.choice_1.emit(s, Message::Choice1Msg);

    let mut buf = vec![0u8; 670 * 480 * 3];

    while app.wait() {
 
          let label: i32 = ui.frame_num_bps.label().parse().unwrap();
          if let Some(msg) = r.recv() {
              match msg {
                    Message::Increment => {
			if pcbp.process_bps(1) == Ok(0) {
                        	ui.frame_num_bps.set_label(&(label +1).to_string());
                    		if pcbp.get_driver_flag() == false {
					pcbp.set_driver_flag();
				}
			}
			else {
				panic!("Add BP error! Error process curve breakpoints!");
			}
		    }
                    Message::Decrement => {
                        if ui.frame_num_bps.label().parse::<i32>().unwrap() >= 3 {
			    if pcbp.process_bps(-1) == Ok(0) {
                                ui.frame_num_bps.set_label(&(label -1).to_string());
			    	if pcbp.get_divider_flag() == false {
				    pcbp.set_divider_flag();
				}
			    }
			}
                    }
		    Message::Choice1Msg => {
		        if pcbp.get_divider_flag() == false {
			    let choice_v = ui.choice_1.choice().unwrap().parse::<f32>();
			    if choice_v != Ok(-1.0) {
				ui.choice_1.set_label(&ui.choice_1.choice().unwrap());
				if choice_v != Ok(pcbp.section_dvder) {
				    let tmp: f32 = choice_v.unwrap();
				    pcbp.set_divider(tmp);
				}
			    }
			    else {
				    panic!("Error, parse choice selection error!");
                	    }
            		}
		    }
	      }
	  }
        
          let root = BitMapBackend::with_buffer(&mut buf, (670/*350 800*/, 480/*300  600*/)).into_drawing_area();     // BMB
          root.fill(&WHITE)?;
          root.margin(10, 10, 10, 10);
          let mut chart = ChartBuilder::on(&root)
			.caption(&pcbp.file_name, ("sans-serif", 40).into_font())
			.x_label_area_size(20)
			.y_label_area_size(60)
                        .build_cartesian_2d(0f32..x_max, 0f32..y_max)?;
            chart.configure_mesh()
			.x_labels(5)
			.y_labels(5)
			.y_label_formatter(&|x| format!("{:.3}", x))
			.draw()?;

            let outtv_cloned = pcbp.orig_bps.clone();
            chart.draw_series(LineSeries::new(
                pcbp.orig_bps.clone(),
                &RED,
            ))?;

    	    chart.draw_series(PointSeries::of_element(
                outtv_cloned,
                2,
                &RED,
                &|c, s, st| {
                    return EmptyElement::at(c)
                    + Circle::new((0,0),s,st.filled())
                    + Text::new(format!("{:?}", c), (10, 0), ("sans-serif", 10).into_font());  //(display the x, y coordinator)
                },
            ))?;
    
            root.present()?;
            drop(chart);
            drop(root);
  
            draw::draw_rgb(&mut ui.frame_for_drawing, &buf).unwrap();
            ui.win.redraw(); 
            app::sleep(0.086);
            app::awake();

    }
    
    app.run().unwrap();

    Ok(())
}
