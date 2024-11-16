use embedded_graphics::image::Image;
use embedded_graphics::pixelcolor::BinaryColor;
use embedded_graphics::prelude::*;

mod images;

use images::*;

pub struct Display {
    pub clk: bool,
    pub bel: bool,
    pub cap: bool,
    pub cmp: bool,
    pub scr: bool,
    pub num: bool,
    pub buzzer: bool,
}

impl Drawable for Display {
    type Color = BinaryColor;
    type Output = ();

    fn draw<D>(&self, target: &mut D) -> Result<Self::Output, D::Error>
    where
        D: DrawTarget<Color = Self::Color>,
    {
        let clk = if self.clk { &CLK_ON } else { &CLK_OFF };
        Image::new(clk, Point::new(78, 0)).draw(target)?;

        let bel = if self.bel { &BEL_ON } else { &BEL_OFF };
        Image::new(bel, Point::new(104, 0)).draw(target)?;

        let cap = if self.cap { &CAP_ON } else { &CAP_OFF };
        Image::new(cap, Point::new(0, 18)).draw(target)?;

        let cmp = if self.cmp { &CMP_ON } else { &CMP_OFF };
        Image::new(cmp, Point::new(26, 18)).draw(target)?;

        let scr = if self.scr { &SCR_ON } else { &SCR_OFF };
        Image::new(scr, Point::new(52, 18)).draw(target)?;

        let num = if self.num { &NUM_ON } else { &NUM_OFF };
        Image::new(num, Point::new(78, 18)).draw(target)?;

        if self.buzzer {
            Image::new(&BUZZER_ON, Point::new(106, 16)).draw(target)?;
        }

        Ok(())
    }
}
