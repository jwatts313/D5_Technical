// 
// ButtonBackend.cs
//  
// Author:
//       Lluis Sanchez <lluis@xamarin.com>
// 
// Copyright (c) 2011 Xamarin Inc
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

using System;
using Xwt.Backends;
using Xwt.Drawing;
using Xwt.CairoBackend;


namespace Xwt.GtkBackend
{
	public partial class ButtonBackend: WidgetBackend, IButtonBackend
	{
		protected bool ignoreClickEvents;
		ImageDescription image;
		ContentPosition contentPosition;
		Pango.FontDescription customFont;
		Gtk.Label labelWidget;
		
		public ButtonBackend ()
		{
		}

		public override void Initialize ()
		{
			NeedsEventBox = false;
			Widget = new Gtk.Button ();
			Widget.Realized += (o, arg) =>
			{
				if (Widget.IsRealized && Widget.CanDefault)
					Widget.GrabDefault();
			};
			base.Widget.Show ();
		}
		
		protected new Gtk.Button Widget {
			get { return (Gtk.Button)base.Widget; }
			set { base.Widget = value; }
		}
		
		protected new IButtonEventSink EventSink {
			get { return (IButtonEventSink)base.EventSink; }
		}

		protected override void OnSetBackgroundColor (Color color)
		{
			Widget.SetBackgroundColor (color);
			Widget.SetBackgroundColor (Gtk.StateType.Prelight, color);
		}

		Color? customLabelColor;

		public virtual Color LabelColor {
			get {
				return customLabelColor.HasValue ? customLabelColor.Value : Gtk.Widget.DefaultStyle.White.ToXwtValue ();
			}
			set {
				customLabelColor = value;
				Widget.SetForegroundColor (value);
				Widget.SetForegroundColor (Gtk.StateType.Prelight, value);
				if (labelWidget != null) {
					labelWidget.SetForegroundColor (value);
					labelWidget.SetForegroundColor (Gtk.StateType.Prelight, value);
				}
			}
		}

		public virtual bool IsDefault {
			get { return Widget.CanDefault; }
			set { Widget.CanDefault = value; }
		}

		public override object Font {
			get {
				return base.Font;
			}
			set {
				base.Font = value;
				customFont = value as Pango.FontDescription;
				SetButtonType (ButtonType.Normal);
			}
		}

		public void SetContent (string label, bool useMnemonic, ImageDescription image, ContentPosition position)
		{
			SetContent (label, useMnemonic, image, position, false);
		}

		void SetContent (string label, bool useMnemonic, ImageDescription image, ContentPosition position, bool forceCustomLabel)
		{			
			Widget.UseUnderline = useMnemonic;
			this.image = image;
			contentPosition = position;
			formattedText = null;

			if (label != null && label.Length == 0)
				label = null;
			
			Button b = (Button) Frontend;
			if (label != null && image.Backend == null && b.Type == ButtonType.Normal && !forceCustomLabel) {
				Widget.Label = label;
				return;
			}
			
			if (b.Type == ButtonType.Disclosure) {
				Widget.Label = null;
				Widget.Image = new Gtk.Arrow (Gtk.ArrowType.Down, Gtk.ShadowType.Out);
				Widget.Image.ShowAll ();
				return;
			}
			
			Gtk.Widget contentWidget = null;
			
			Gtk.Widget imageWidget = null;
			if (image.Backend != null)
				imageWidget = new ImageBox (ApplicationContext, image.WithDefaultSize (Gtk.IconSize.Button));

			if (labelWidget != null)
			{
				labelWidget.Realized -= HandleStyleUpdate;
				labelWidget.StyleSet -= HandleStyleUpdate;
				labelWidget = null;
			}

			if (label != null && imageWidget == null) {
				contentWidget = labelWidget = new Gtk.Label (label);
			}
			else if (label == null && imageWidget != null) {
				contentWidget = imageWidget;
			}
			else if (label != null && imageWidget != null) {
				Gtk.Box box = position == ContentPosition.Left || position == ContentPosition.Right ? (Gtk.Box) new Gtk.HBox (false, 3) : (Gtk.Box) new Gtk.VBox (false, 3);
				labelWidget = new Gtk.Label (label) { UseUnderline = useMnemonic };
				
				if (position == ContentPosition.Left || position == ContentPosition.Top) {
					box.PackStart (imageWidget, false, false, 0);
					box.PackStart (labelWidget, false, false, 0);
				} else {
					box.PackStart (labelWidget, false, false, 0);
					box.PackStart (imageWidget, false, false, 0);
				}
				
				contentWidget = box;
			}
			var expandButtonContent = false;
			if (b.Type == ButtonType.DropDown) {
				if (contentWidget != null) {
					Gtk.HBox box = new Gtk.HBox (false, 3);
					box.PackStart (contentWidget, true, true, 3);
					box.PackStart (new Gtk.Arrow (Gtk.ArrowType.Down, Gtk.ShadowType.Out), false, false, 0);
					contentWidget = box;
					expandButtonContent = true;
				} else
					contentWidget = new Gtk.Arrow (Gtk.ArrowType.Down, Gtk.ShadowType.Out);
			}
			if (contentWidget != null) {
				contentWidget.ShowAll ();
				Widget.Label = null;
				Widget.Image = contentWidget;
				var alignment = Widget.Child as Gtk.Alignment;
				if (alignment != null) {
					if (expandButtonContent) {
						var box = alignment.Child as Gtk.Box;
						if (box != null) {
							alignment.Xscale = 1;
							box.SetChildPacking (box.Children [0], true, true, 0, Gtk.PackType.Start);
							if (labelWidget != null)
								labelWidget.Xalign = 0;
						}
					} else if (position == ContentPosition.Left && (contentWidget is Gtk.Box)) {
						// in case the button is wider than its natural size and has text and an image on the left,
						// optimize its alignment to make the text more centered.
						// FIXME: more sophisticated size calculation
						alignment.Xalign = 0.475f;
					}
				}
				if (labelWidget != null) {
					labelWidget.UseUnderline = useMnemonic;
					if (customFont != null)
						labelWidget.ModifyFont (customFont);
					if (customLabelColor.HasValue) {
						labelWidget.SetForegroundColor (customLabelColor.Value);
						labelWidget.SetForegroundColor (Gtk.StateType.Prelight, customLabelColor.Value);
					}
				}
			} else
				Widget.Label = null;
		}

		public void SetButtonStyle (ButtonStyle style)
		{
			switch (style) {
			case ButtonStyle.Normal:
				SetMiniMode (false);
				Widget.Relief = Gtk.ReliefStyle.Normal;
				break;
			case ButtonStyle.Flat:
				SetMiniMode (false);
				Widget.Relief = Gtk.ReliefStyle.None;
				break;
			case ButtonStyle.Borderless:
				SetMiniMode (true);
				Widget.Relief = Gtk.ReliefStyle.None;
				break;
			}
		}
		
		public void SetButtonType (ButtonType type)
		{
			Button b = (Button) Frontend;
			SetContent (b.Label, b.UseMnemonic, image, b.ImagePosition);
		}

		FormattedText formattedText;
		public void SetFormattedText (FormattedText text)
		{
			// set content with a custom label, this will recreate labelWidget
			SetContent (text?.Text, Widget.UseUnderline, image, contentPosition, true);
			formattedText = text;
			if (!string.IsNullOrEmpty (formattedText?.Text))
			{
				if (labelWidget.IsRealized)
					labelWidget.ApplyFormattedText (formattedText);
				else
					labelWidget.Realized += HandleStyleUpdate;
				labelWidget.StyleSet += HandleStyleUpdate;
			}
		}

		void HandleStyleUpdate (object sender, EventArgs e)
		{
			// force text update with updated link color
			if (labelWidget.IsRealized && formattedText != null)
			{
				labelWidget.ApplyFormattedText (formattedText);
			}
		}

		public override void EnableEvent (object eventId)
		{
			base.EnableEvent (eventId);
			if (eventId is ButtonEvent) {
				switch ((ButtonEvent)eventId) {
				case ButtonEvent.Clicked: Widget.Clicked += HandleWidgetClicked; break;
				}
			}
		}
		
		public override void DisableEvent (object eventId)
		{
			base.DisableEvent (eventId);
			if (eventId is ButtonEvent) {
				switch ((ButtonEvent)eventId) {
				case ButtonEvent.Clicked: Widget.Clicked -= HandleWidgetClicked; break;
				}
			}
		}

		void HandleWidgetClicked (object sender, EventArgs e)
		{
			if (!ignoreClickEvents) {
				ApplicationContext.InvokeUserCode (EventSink.OnClicked);
			}
		}
		
		bool miniMode;
		
		protected void SetMiniMode (bool miniMode)
		{
//			Gtk.Rc.ParseString ("style \"Xwt.GtkBackend.CustomButton\" {\n GtkButton::inner-border = {0,0,0,0} GtkButton::child-displacement-x = {0} GtkButton::child-displacement-y = {0}\n }\n");
//			Gtk.Rc.ParseString ("widget \"*.Xwt.GtkBackend.CustomButton\" style  \"Xwt.GtkBackend.CustomButton\"\n");
//			Name = "Xwt.GtkBackend.CustomButton";
			
			if (this.miniMode == miniMode)
				return;
			this.miniMode = miniMode;
			if (miniMode) {
				Widget.SizeAllocated += HandleSizeAllocated;
			}
			SetMiniModeGtk(miniMode);
			Widget.QueueResize ();
		}

		[GLib.ConnectBefore]
		void HandleSizeAllocated (object o, Gtk.SizeAllocatedArgs args)
		{
			Widget.Child.SizeAllocate (args.Allocation);
			args.RetVal = true;
		}

		protected override void Dispose (bool disposing)
		{
			if (disposing && labelWidget != null) {
				labelWidget.Realized -= HandleStyleUpdate;
				labelWidget.StyleSet -= HandleStyleUpdate;
				labelWidget.Destroy ();
				labelWidget = null;
			}
			base.Dispose (disposing);
		}
	}
}

