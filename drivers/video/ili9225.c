/*
 * ILI9225/ILI9226 Framebuffer driver
 *
 * Copyright (c) 2009 Jean-Christian de Rivaz
 * Copyright (c) 2011 Miguel Angel Ajo Pelayo
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * The Terance Semiconductor ili9225 chip drive TFT screen up to 176x220. This
 * driver expect the ili9225 to be connected to a 16 bits local bus and
 * to be set in the 16 bits parallel interface mode. To use it you must
 * define in your board file a struct platform_device with a name set to
 * "ili9225" and a struct resource array with two IORESOURCE_MEM: the first
 * for the control register; the second for the data register.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <mach/gpio.h>



struct ili9225_page {
	unsigned short x;
	unsigned short y;
	unsigned short *buffer;
	unsigned short len;
};

struct ili9225 {
	struct device *dev;
	volatile unsigned short *ctrl_io;
	volatile unsigned short *data_io;
	struct fb_info *info;
	unsigned int pages_count;
	struct ili9225_page *pages;
	unsigned short *last_buffer_start;
};

static inline void ili9225_reg_set(struct ili9225 *item, unsigned char reg,
				   unsigned short value)
{
	unsigned short ctrl = reg & 0x00ff;

	writew(ctrl, item->ctrl_io);
	writew(value, item->data_io);
}

static inline void ili9225_send_cmd_slow(struct ili9225 *item, unsigned char reg)
{
	unsigned short ctrl = reg & 0x00ff;

	writew(ctrl, item->ctrl_io);
	mdelay(1);
	
}

static inline void ili9225_send_data_slow(struct ili9225 *item, unsigned short value)
{
	writew(value, item->data_io);
	mdelay(1);
	
}

static inline unsigned short ili9225_read_data_slow(struct ili9225 *item)
{
	mdelay(1);
	return readw(item->data_io);

	
}


static inline unsigned short ili9225_read_data(struct ili9225 *item)
{

	return readw(item->data_io);

	
}


static inline void ili9225_send_cmd(struct ili9225 *item, unsigned char reg)
{
	unsigned short ctrl = reg & 0x00ff;

	writew(ctrl, item->ctrl_io);
	
}

static inline void ili9225_send_data(struct ili9225 *item, unsigned char value)
{
	writew(value, item->data_io);
}


static void ili9225_copy(struct ili9225 *item, unsigned int index)
{
	unsigned short x;
	unsigned short y;
	unsigned short *buffer;
	unsigned int len;
//	unsigned int count;
	unsigned int diff;

	x = item->pages[index].x;
	y = item->pages[index].y;

	buffer = item->pages[index].buffer;
	len = item->pages[index].len;

	// check if we should change the chip framebuffer pointer
	if (item->last_buffer_start!=buffer)
	{

/*
		if (x!=0)	// The ssd1963 needs to start from X=0 to keep drawing in next lines 
		{
			diff = x*(item->info->var.bits_per_pixel / 16);
			buffer-=diff;
			len+=diff;
		}
	*/

		

		dev_dbg(item->dev, 
			"%s: page[%u]: x=%3hu y=%3hu buffer=0x%p len=%3hu diff=%3hu\n",
			__func__, index, x, y, buffer, len,diff);

		ili9225_reg_set(item,0x0003,0x1038); // set GRAM write direction and BG and horizontal mode
		ili9225_reg_set(item,0x20,y);
		ili9225_reg_set(item,0x21,x);
		ili9225_send_cmd (item, 0x22);
	}


	memcpy((void*)(item->data_io),buffer,len*2);
	item->last_buffer_start=&buffer[len];
}

static void ili9225_update(struct fb_info *info, struct list_head *pagelist)
{
	struct ili9225 *item = (struct ili9225 *)info->par;
	struct page *page;

	list_for_each_entry(page, pagelist, lru) {
		ili9225_copy(item, page->index);
	}
}

static void __init ili9225_update_all(struct ili9225 *item)
{
	unsigned short index;

	for (index = 0; index < item->pages_count; index++) {
		ili9225_copy(item, index);
	}
}

static void __init ili9225_setup(struct ili9225 *item)
{
	
	
	dev_dbg(item->dev, "%s: item=0x%p\n", __func__, (void *)item);
		
	#define LCD_CtrlWrite_ILI9225G(reg_n,data) ili9225_reg_set(item,reg_n,data)  
	#define delayms mdelay
	
	//************* Start Initial Sequence **********//
#ifdef LCD_3V3
printk("LCD in 3.3V mode\n");
	LCD_CtrlWrite_ILI9225G(0x00D0, 0x0003);
	LCD_CtrlWrite_ILI9225G(0x00EB, 0x0B00);
	LCD_CtrlWrite_ILI9225G(0x00EC, 0x000F);
	LCD_CtrlWrite_ILI9225G(0x00C7, 0x030F);
	LCD_CtrlWrite_ILI9225G(0x0001, 0x001C); // GS=0 (bit9) to reverse left/right flip
	LCD_CtrlWrite_ILI9225G(0x0002, 0x0100);
	LCD_CtrlWrite_ILI9225G(0x0003, 0x1030);
	LCD_CtrlWrite_ILI9225G(0x0008, 0x0808);
	LCD_CtrlWrite_ILI9225G(0x000F, 0x0901);

	delayms(10);

	LCD_CtrlWrite_ILI9225G(0x0010, 0x0000);
	LCD_CtrlWrite_ILI9225G(0x0011, 0x1B41); //The register setting is suitable for VCI=2.8V

	delayms(120);

	LCD_CtrlWrite_ILI9225G(0x0012, 0x200E);//The register setting is suitable for VCI=3.3V
	LCD_CtrlWrite_ILI9225G(0x0013, 0x0052); //The register setting is suitable for VCI=3.3V
	LCD_CtrlWrite_ILI9225G(0x0014, 0x4B5C); //The register setting is suitable for VCI=3.3V

	LCD_CtrlWrite_ILI9225G(0x0030, 0x0000);
	LCD_CtrlWrite_ILI9225G(0x0031, 0x00DB);
	LCD_CtrlWrite_ILI9225G(0x0032, 0x0000);
	LCD_CtrlWrite_ILI9225G(0x0033, 0x0000);
	LCD_CtrlWrite_ILI9225G(0x0034, 0x00DB);
	LCD_CtrlWrite_ILI9225G(0x0035, 0x0000);
	LCD_CtrlWrite_ILI9225G(0x0036, 0x00AF);
	LCD_CtrlWrite_ILI9225G(0x0037, 0x0000);
	LCD_CtrlWrite_ILI9225G(0x0038, 0x00DB);
	LCD_CtrlWrite_ILI9225G(0x0039, 0x0000);
	
	LCD_CtrlWrite_ILI9225G(0x0050, 0x0000);
	LCD_CtrlWrite_ILI9225G(0x0051, 0x0705);
	LCD_CtrlWrite_ILI9225G(0x0052, 0x0C0A);
	LCD_CtrlWrite_ILI9225G(0x0053, 0x0401);
	LCD_CtrlWrite_ILI9225G(0x0054, 0x040C);
	LCD_CtrlWrite_ILI9225G(0x0055, 0x0608);
	LCD_CtrlWrite_ILI9225G(0x0056, 0x0000);
	LCD_CtrlWrite_ILI9225G(0x0057, 0x0104);
	LCD_CtrlWrite_ILI9225G(0x0058, 0x0E06);
	LCD_CtrlWrite_ILI9225G(0x0059, 0x060E);
#else
	//************* Start Initial Sequence **********// 

printk("LCD in 2.8V mode\n");
LCD_CtrlWrite_ILI9225G(0x00D0, 0x0003);     
LCD_CtrlWrite_ILI9225G(0x00EB, 0x0B00);     
LCD_CtrlWrite_ILI9225G(0x00EC, 0x004F);      
LCD_CtrlWrite_ILI9225G(0x00C7, 0x003F);      
LCD_CtrlWrite_ILI9225G(0x0001, 0x001C); // GS=0 (bit9) to reverse left/right flip     
LCD_CtrlWrite_ILI9225G(0x0002, 0x0100);    
LCD_CtrlWrite_ILI9225G(0x0003, 0x1030);      
LCD_CtrlWrite_ILI9225G(0x0008, 0x0808);     
LCD_CtrlWrite_ILI9225G(0x000F, 0x0A01);  
delayms(10);     
LCD_CtrlWrite_ILI9225G(0x0010, 0x0000);     
LCD_CtrlWrite_ILI9225G(0x0011, 0x1B41); //The register setting is suitable for VCI=2.8V 
delayms(120);     
LCD_CtrlWrite_ILI9225G(0x0012, 0x300E);//The register setting is suitable for VCI=2.8V     
LCD_CtrlWrite_ILI9225G(0x0013, 0x0057); //The register setting is suitable for VCI=2.8V     
LCD_CtrlWrite_ILI9225G(0x0014, 0x516B); //The register setting is suitable for VCI=2.8V      
LCD_CtrlWrite_ILI9225G(0x0030, 0x0000);      
LCD_CtrlWrite_ILI9225G(0x0031, 0x00DB);      
LCD_CtrlWrite_ILI9225G(0x0032, 0x0000);  
LCD_CtrlWrite_ILI9225G(0x0033, 0x0000);      
LCD_CtrlWrite_ILI9225G(0x0034, 0x00DB);      
LCD_CtrlWrite_ILI9225G(0x0035, 0x0000);      
LCD_CtrlWrite_ILI9225G(0x0036, 0x00AF);      
LCD_CtrlWrite_ILI9225G(0x0037, 0x0000);      
LCD_CtrlWrite_ILI9225G(0x0038, 0x00DB);      
LCD_CtrlWrite_ILI9225G(0x0039, 0x0000);
LCD_CtrlWrite_ILI9225G(0x0050, 0x0000);     
LCD_CtrlWrite_ILI9225G(0x0051, 0x060A);     
LCD_CtrlWrite_ILI9225G(0x0052, 0x0C08);     
LCD_CtrlWrite_ILI9225G(0x0053, 0x0400);     
LCD_CtrlWrite_ILI9225G(0x0054, 0x080C);     
LCD_CtrlWrite_ILI9225G(0x0055, 0x0B05);     
LCD_CtrlWrite_ILI9225G(0x0056, 0x0000);     
LCD_CtrlWrite_ILI9225G(0x0057, 0x0004);     
LCD_CtrlWrite_ILI9225G(0x0058, 0x0000);     
LCD_CtrlWrite_ILI9225G(0x0059, 0x0000);
#endif       
        LCD_CtrlWrite_ILI9225G(0x0003, 0x1038); // set GRAM write direction and BG and horizontal mode  
                    
	LCD_CtrlWrite_ILI9225G(0x0020, 0x0000);
	LCD_CtrlWrite_ILI9225G(0x0021, 0x0000);
	LCD_CtrlWrite_ILI9225G(0x0007, 0x1017);

	ili9225_send_cmd(item,0x22);
	
}

void ili9225_enter_standby(struct ili9225 *item)
{
	LCD_CtrlWrite_ILI9225G(0x0007, 0x0000);
	delayms(50);
	LCD_CtrlWrite_ILI9225G(0x0010, 0x0001);
}
void ili9225_exit_standby(struct ili9225 *item)
{
	LCD_CtrlWrite_ILI9225G(0x0010, 0x0000);
	delayms(120);
	LCD_CtrlWrite_ILI9225G(0x0007, 0x1017);
}

static int __init ili9225_video_alloc(struct ili9225 *item)
{
	unsigned int frame_size;

	dev_dbg(item->dev, "%s: item=0x%p\n", __func__, (void *)item);

	frame_size = item->info->fix.line_length * item->info->var.yres;
	dev_dbg(item->dev, "%s: item=0x%p frame_size=%u\n",
		__func__, (void *)item, frame_size);

	item->pages_count = frame_size / PAGE_SIZE;
	if ((item->pages_count * PAGE_SIZE) < frame_size) {
		item->pages_count++;
	}
	dev_dbg(item->dev, "%s: item=0x%p pages_count=%u\n",
		__func__, (void *)item, item->pages_count);

	item->info->fix.smem_len = item->pages_count * PAGE_SIZE;
	item->info->fix.smem_start =
	    (unsigned long)vmalloc(item->info->fix.smem_len);
	if (!item->info->fix.smem_start) {
		dev_err(item->dev, "%s: unable to vmalloc\n", __func__);
		return -ENOMEM;
	}
	memset((void *)item->info->fix.smem_start, 0, item->info->fix.smem_len);

	return 0;
}

static void ili9225_video_free(struct ili9225 *item)
{
	dev_dbg(item->dev, "%s: item=0x%p\n", __func__, (void *)item);

	kfree((void *)item->info->fix.smem_start);
}

static int __init ili9225_pages_alloc(struct ili9225 *item)
{
	unsigned short pixels_per_page;
	unsigned short yoffset_per_page;
	unsigned short xoffset_per_page;
	unsigned short index;
	unsigned short x = 0;
	unsigned short y = 0;
	unsigned short *buffer;
	unsigned int len;

	dev_dbg(item->dev, "%s: item=0x%p\n", __func__, (void *)item);

	item->pages = kmalloc(item->pages_count * sizeof(struct ili9225_page),
			      GFP_KERNEL);
	if (!item->pages) {
		dev_err(item->dev, "%s: unable to kmalloc for ili9225_page\n",
			__func__);
		return -ENOMEM;
	}

	pixels_per_page = PAGE_SIZE / (item->info->var.bits_per_pixel / 8);
	yoffset_per_page = pixels_per_page / item->info->var.xres;
	xoffset_per_page = pixels_per_page -
	    (yoffset_per_page * item->info->var.xres);
	dev_dbg(item->dev, "%s: item=0x%p pixels_per_page=%hu "
		"yoffset_per_page=%hu xoffset_per_page=%hu\n",
		__func__, (void *)item, pixels_per_page,
		yoffset_per_page, xoffset_per_page);

	buffer = (unsigned short *)item->info->fix.smem_start;
	for (index = 0; index < item->pages_count; index++) {
		len = (item->info->var.xres * item->info->var.yres) -
		    (index * pixels_per_page);
		if (len > pixels_per_page) {
			len = pixels_per_page;
		}
		dev_dbg(item->dev,
			"%s: page[%d]: x=%3hu y=%3hu buffer=0x%p len=%3hu\n",
			__func__, index, x, y, buffer, len);
		item->pages[index].x = x;
		item->pages[index].y = y;
		item->pages[index].buffer = buffer;
		item->pages[index].len = len;

		x += xoffset_per_page;
		if (x >= item->info->var.xres) {
			y++;
			x -= item->info->var.xres;
		}
		y += yoffset_per_page;
		buffer += pixels_per_page;
	}

	/* tell the copy routines that the LCD controller is ready to receive data
 	  from the start of the buffer X,Y = (0,0) */

	item->last_buffer_start=(unsigned short *)item->info->fix.smem_start;

	return 0;
}

static void ili9225_pages_free(struct ili9225 *item)
{
	dev_dbg(item->dev, "%s: item=0x%p\n", __func__, (void *)item);

	kfree(item->pages);
}

static struct fb_ops ili9225_fbops = {
	.owner        = THIS_MODULE,
	.fb_fillrect  = sys_fillrect,
	.fb_copyarea  = sys_copyarea,
	.fb_imageblit = sys_imageblit,
};

static struct fb_fix_screeninfo ili9225_fix __initdata = {
	.id          = "ili9225",
	.type        = FB_TYPE_PACKED_PIXELS,
	.visual      = FB_VISUAL_DIRECTCOLOR,
	.accel       = FB_ACCEL_NONE,
	.line_length = 220 * 2,
};

static struct fb_var_screeninfo ili9225_var __initdata = {
	.xres		= 220,
	.yres		= 176,
	.xres_virtual	= 220,
	.yres_virtual	= 176,
	.bits_per_pixel	= 16,
	.red		= {6, 5, 0},
	.green		= {11, 5, 0},
	.blue		= {0, 6, 0},
	
	.activate	= FB_ACTIVATE_NOW,
	.height		= 220,
	.width		= 176,
	.vmode		= FB_VMODE_NONINTERLACED,
};

static struct fb_deferred_io ili9225_defio = {
        .delay          = HZ / 50,
        .deferred_io    = &ili9225_update,
};

static int __init ili9225_probe(struct platform_device *dev)
{
	int ret = 0;
	struct ili9225 *item;
	struct resource *ctrl_res;
	struct resource *data_res;
	unsigned int ctrl_res_size=0;
	unsigned int data_res_size=0;
	struct resource *ctrl_req;
	struct resource *data_req;

	struct fb_info *info;
	unsigned short int id[6];

	dev_dbg(&dev->dev, "%s\n", __func__);

	item = kzalloc(sizeof(struct ili9225), GFP_KERNEL);
	if (!item) {
		dev_err(&dev->dev,
			"%s: unable to kzalloc for ili9225\n", __func__);
		ret = -ENOMEM;
		goto out;
	}
	item->dev = &dev->dev;
	dev_set_drvdata(&dev->dev, item);

	ctrl_res = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!ctrl_res) {
		dev_err(&dev->dev,
			"%s: unable to platform_get_resource for ctrl_res\n",
			__func__);
		ret = -ENOENT;
		goto out_no_release;
	}
	ctrl_res_size = ctrl_res->end - ctrl_res->start + 1;
	ctrl_req = request_mem_region(ctrl_res->start, ctrl_res_size,
				      dev->name);
	if (!ctrl_req) {
		dev_err(&dev->dev,
			"%s: unable to request_mem_region for ctrl_req\n",
			__func__);
		ret = -EIO;
		goto out_no_release;
	}
	
	/* Now that we're sure that we own the memory bus... reset timings */
	MPMC_STCONFIG0 = 0x81;
	MPMC_STWTWEN0  = 5;
	MPMC_STWTOEN0  = 5;
	MPMC_STWTRD0   = 31;
	MPMC_STWTPG0   = 5;
	MPMC_STWTWR0   = 10;
	MPMC_STWTTURN0 = 8;
	
	
	
	item->ctrl_io = ioremap(ctrl_res->start, ctrl_res_size);
	if (!item->ctrl_io) {
		ret = -EINVAL;
		dev_err(&dev->dev,
			"%s: unable to ioremap for ctrl_io\n", __func__);
		goto out_item;
	}

	data_res = platform_get_resource(dev, IORESOURCE_MEM, 1);
	if (!data_res) {
		dev_err(&dev->dev,
			"%s: unable to platform_get_resource for data_res\n",
			__func__);
		ret = -ENOENT;
		goto out_item;
	}
	data_res_size = data_res->end - data_res->start + 1;
	data_req = request_mem_region(data_res->start,
				      data_res_size, dev->name);
	if (!data_req) {
		dev_err(&dev->dev,
			"%s: unable to request_mem_region for data_req\n",
			__func__);
		ret = -EIO;
		goto out_item;
	}
	item->data_io = ioremap(data_res->start, data_res_size);
	if (!item->data_io) {
		ret = -EINVAL;
		dev_err(&dev->dev,
			"%s: unable to ioremap for data_io\n", __func__);
		goto out_item;
	}

	dev_dbg(&dev->dev, "%s: ctrl_io=%p data_io=%p\n",
		__func__, item->ctrl_io, item->data_io);


	ili9225_reg_set(item,0x007e,0x0000); // FCH(00)
	ili9225_send_cmd(item,0x0000);
	id[0] = ili9225_read_data(item);
  
	dev_dbg(&dev->dev, "%s: signature=%04x\n", __func__, id[0]);

	if ((id[0]!=0x9225)&&(id[0]!=0x9226)) {
		ret = -ENODEV;
		dev_err(&dev->dev,
			"%s: unknown signature %04x\n", __func__, id[0]);
		goto out_item;
	}
	
	dev_info(&dev->dev, "%s: signature=%04x\n",__func__,id[0]);

	dev_info(&dev->dev, "item=0x%p ctrl=0x%p data=0x%p\n", (void *)item,
		 (void *)ctrl_res->start, (void *)data_res->start);

	info = framebuffer_alloc(sizeof(struct ili9225), &dev->dev);
	if (!info) {
		ret = -ENOMEM;
		dev_err(&dev->dev,
			"%s: unable to framebuffer_alloc\n", __func__);
		goto out_item;
	}
	item->info = info;
	info->par = item;
	info->fbops = &ili9225_fbops;
	info->flags = FBINFO_FLAG_DEFAULT;
	info->fix = ili9225_fix;
	info->var = ili9225_var;

	ret = ili9225_video_alloc(item);
	if (ret) {
		dev_err(&dev->dev,
			"%s: unable to ili9225_video_alloc\n", __func__);
		goto out_info;
	}
	info->screen_base = (char __iomem *)item->info->fix.smem_start;

	ret = ili9225_pages_alloc(item);
	if (ret < 0) {
		dev_err(&dev->dev,
			"%s: unable to ili9225_pages_init\n", __func__);
		goto out_video;
	}
	info->fbdefio = &ili9225_defio;
	fb_deferred_io_init(info);

	ret = register_framebuffer(info);
	if (ret < 0) {
		dev_err(&dev->dev,
			"%s: unable to register_frambuffer\n", __func__);
		goto out_pages;
	}
	ili9225_setup(item);
	ili9225_update_all(item);
	
	lpc31xx_gpio_set_value(GPIO_PWM_DATA,1); //switch on backlight

	return ret;

out_pages:
	ili9225_pages_free(item);
out_video:
	ili9225_video_free(item);
out_info:
	framebuffer_release(info);
out_item:
	release_mem_region(data_res->start,	data_res_size);
	release_mem_region(ctrl_res->start,	ctrl_res_size);
out_no_release:
	kfree(item);
out:
	return ret;
}

static struct platform_driver ili9225_driver = {
	.probe = ili9225_probe,
	.driver = {
		   .name = "ili9225",
		   },
};

static int __init ili9225_init(void)
{
	int ret = 0;

	pr_debug("%s\n", __func__);

	ret = platform_driver_register(&ili9225_driver);
	if (ret) {
		pr_err("%s: unable to platform_driver_register\n", __func__);
	}

	return ret;
}

module_init(ili9225_init);

MODULE_DESCRIPTION("ili9225 LCD Driver");
MODULE_AUTHOR("Miguel Angel Ajo Pelayo <miguelangel@nbee.es>");
MODULE_LICENSE("GPL");
