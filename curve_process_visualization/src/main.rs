/* 
 * Juntong Liu
 *         2023.Jun 28
 * 
 * File name: main.rs   
 * version 0.1
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

    let mut buf = vec![0u8; 670 * 480 * 3];

    while app.wait() {
 
          let label: i32 = ui.frame_num_bps.label().parse().unwrap();
          if let Some(msg) = r.recv() {
              match msg {
                    Message::Increment => {
                        ui.frame_num_bps.set_label(&(label +1).to_string());
                    }
                    Message::Decrement => {
                        if ui.frame_num_bps.label().parse::<i32>().unwrap() > 2 {
                            ui.frame_num_bps.set_label(&(label -1).to_string());
                        }
                    }
                }
            }
            let num_bps_to_update = ui.frame_num_bps.label().parse::<i32>().unwrap() - label;  
            if pcbp.process_bps(num_bps_to_update) != Ok(0) {
                panic!("Error process curve breakpoints!");
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
