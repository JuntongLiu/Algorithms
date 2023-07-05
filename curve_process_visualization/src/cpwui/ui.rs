
#![allow(unused_variables)]
#![allow(unused_mut)]
#![allow(unused_imports)]
#![allow(dead_code)]
#![allow(clippy::needless_update)]

use fltk::browser::*;
use fltk::button::*;
use fltk::dialog::*;
use fltk::enums::*;
use fltk::frame::*;
use fltk::group::*;
use fltk::image::*;
use fltk::input::*;
use fltk::menu::*;
use fltk::misc::*;
use fltk::output::*;
use fltk::prelude::*;
use fltk::table::*;
use fltk::text::*;
use fltk::tree::*;
use fltk::valuator::*;
use fltk::widget::*;
use fltk::window::*;

#[derive(Debug, Clone)]
pub struct UserInterface {
    pub win: Window,
    pub frame_for_drawing: Frame,
    pub frame_num_bps: Frame,
    pub minus_but: Button,
    pub plus_but: Button,
}

impl UserInterface {
    pub fn make_window() -> Self {
	let mut win = Window::new(64, 112, 850, 500, None);
	win.set_label("Curve Operation");
	win.end();
	win.set_type(WindowType::Double);
	win.make_resizable(true);
	win.show();
	let mut frame_for_drawing = Frame::new(10, 10, 670, 480, None);
	frame_for_drawing.set_label("label");
	win.add(&frame_for_drawing);
	let mut fl2rust_widget_0 = Frame::new(705, 160, 120, 30, None);
	fl2rust_widget_0.set_label("Number of BPs:");
	win.add(&fl2rust_widget_0);
	let mut frame_num_bps = Frame::new(705, 195, 120, 30, None);
	frame_num_bps.set_label("0");
	win.add(&frame_num_bps);
	let mut minus_but = Button::new(705, 235, 50, 30, None);
	minus_but.set_label("-");
	win.add(&minus_but);
	let mut plus_but = Button::new(775, 235, 50, 30, None);
	plus_but.set_label("+");
	win.add(&plus_but);
	Self {
	    win,
	    frame_for_drawing,
	    frame_num_bps,
	    minus_but,
	    plus_but,
	}
  }
}
