namespace TitanExplorer
{
    partial class logForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(logForm));
            this.logPanel = new System.Windows.Forms.Panel();
            this.alwaysOnTopCheckBox = new DarkUI.Controls.DarkCheckBox();
            this.logTimestampCheckBox = new DarkUI.Controls.DarkCheckBox();
            this.logBufferProgressBar = new System.Windows.Forms.ProgressBar();
            this.logTextBoxHidden = new DarkUI.Controls.DarkTextBox();
            this.logFreezeCheckBox = new DarkUI.Controls.DarkCheckBox();
            this.logClearButton = new DarkUI.Controls.DarkButton();
            this.logSaveButton = new DarkUI.Controls.DarkButton();
            this.logTextBox = new DarkUI.Controls.DarkTextBox();
            this.saveFileDialog = new System.Windows.Forms.SaveFileDialog();
            this.logPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // logPanel
            // 
            this.logPanel.Controls.Add(this.alwaysOnTopCheckBox);
            this.logPanel.Controls.Add(this.logTimestampCheckBox);
            this.logPanel.Controls.Add(this.logBufferProgressBar);
            this.logPanel.Controls.Add(this.logTextBoxHidden);
            this.logPanel.Controls.Add(this.logFreezeCheckBox);
            this.logPanel.Controls.Add(this.logClearButton);
            this.logPanel.Controls.Add(this.logSaveButton);
            this.logPanel.Controls.Add(this.logTextBox);
            this.logPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.logPanel.Location = new System.Drawing.Point(0, 0);
            this.logPanel.Name = "logPanel";
            this.logPanel.Size = new System.Drawing.Size(800, 450);
            this.logPanel.TabIndex = 3;
            // 
            // alwaysOnTopCheckBox
            // 
            this.alwaysOnTopCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.alwaysOnTopCheckBox.AutoSize = true;
            this.alwaysOnTopCheckBox.Location = new System.Drawing.Point(153, 426);
            this.alwaysOnTopCheckBox.Name = "alwaysOnTopCheckBox";
            this.alwaysOnTopCheckBox.Size = new System.Drawing.Size(96, 17);
            this.alwaysOnTopCheckBox.TabIndex = 10;
            this.alwaysOnTopCheckBox.Text = "Always on Top";
            this.alwaysOnTopCheckBox.CheckedChanged += new System.EventHandler(this.alwaysOnTopCheckBox_CheckedChanged);
            // 
            // logTimestampCheckBox
            // 
            this.logTimestampCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.logTimestampCheckBox.AutoSize = true;
            this.logTimestampCheckBox.Location = new System.Drawing.Point(70, 426);
            this.logTimestampCheckBox.Name = "logTimestampCheckBox";
            this.logTimestampCheckBox.Size = new System.Drawing.Size(77, 17);
            this.logTimestampCheckBox.TabIndex = 9;
            this.logTimestampCheckBox.Text = "Timestamp";
            // 
            // logBufferProgressBar
            // 
            this.logBufferProgressBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.logBufferProgressBar.Location = new System.Drawing.Point(255, 422);
            this.logBufferProgressBar.Name = "logBufferProgressBar";
            this.logBufferProgressBar.Size = new System.Drawing.Size(380, 23);
            this.logBufferProgressBar.TabIndex = 8;
            // 
            // logTextBoxHidden
            // 
            this.logTextBoxHidden.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.logTextBoxHidden.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(69)))), ((int)(((byte)(73)))), ((int)(((byte)(74)))));
            this.logTextBoxHidden.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.logTextBoxHidden.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(220)))));
            this.logTextBoxHidden.Location = new System.Drawing.Point(96, 12);
            this.logTextBoxHidden.MaxLength = 33553408;
            this.logTextBoxHidden.Multiline = true;
            this.logTextBoxHidden.Name = "logTextBoxHidden";
            this.logTextBoxHidden.Size = new System.Drawing.Size(620, 41);
            this.logTextBoxHidden.TabIndex = 7;
            this.logTextBoxHidden.Visible = false;
            // 
            // logFreezeCheckBox
            // 
            this.logFreezeCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.logFreezeCheckBox.AutoSize = true;
            this.logFreezeCheckBox.Location = new System.Drawing.Point(6, 426);
            this.logFreezeCheckBox.Name = "logFreezeCheckBox";
            this.logFreezeCheckBox.Size = new System.Drawing.Size(58, 17);
            this.logFreezeCheckBox.TabIndex = 6;
            this.logFreezeCheckBox.Text = "Freeze";
            // 
            // logClearButton
            // 
            this.logClearButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.logClearButton.Location = new System.Drawing.Point(722, 422);
            this.logClearButton.Name = "logClearButton";
            this.logClearButton.Padding = new System.Windows.Forms.Padding(5);
            this.logClearButton.Size = new System.Drawing.Size(75, 23);
            this.logClearButton.TabIndex = 3;
            this.logClearButton.Text = "Clear";
            this.logClearButton.Click += new System.EventHandler(this.logClearButton_Click);
            // 
            // logSaveButton
            // 
            this.logSaveButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.logSaveButton.Location = new System.Drawing.Point(641, 422);
            this.logSaveButton.Name = "logSaveButton";
            this.logSaveButton.Padding = new System.Windows.Forms.Padding(5);
            this.logSaveButton.Size = new System.Drawing.Size(75, 23);
            this.logSaveButton.TabIndex = 2;
            this.logSaveButton.Text = "Save";
            this.logSaveButton.Click += new System.EventHandler(this.logSaveButton_Click);
            // 
            // logTextBox
            // 
            this.logTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.logTextBox.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(69)))), ((int)(((byte)(73)))), ((int)(((byte)(74)))));
            this.logTextBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.logTextBox.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(220)))));
            this.logTextBox.Location = new System.Drawing.Point(0, 0);
            this.logTextBox.MaxLength = 33553408;
            this.logTextBox.Multiline = true;
            this.logTextBox.Name = "logTextBox";
            this.logTextBox.ReadOnly = true;
            this.logTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.logTextBox.Size = new System.Drawing.Size(800, 416);
            this.logTextBox.TabIndex = 1;
            // 
            // logForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(800, 450);
            this.Controls.Add(this.logPanel);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "logForm";
            this.Text = "TITAN Explorer Log";
            this.Resize += new System.EventHandler(this.logForm_Resize);
            this.logPanel.ResumeLayout(false);
            this.logPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel logPanel;
        private System.Windows.Forms.ProgressBar logBufferProgressBar;
        private System.Windows.Forms.SaveFileDialog saveFileDialog;
        private DarkUI.Controls.DarkTextBox logTextBoxHidden;
        private DarkUI.Controls.DarkCheckBox logFreezeCheckBox;
        private DarkUI.Controls.DarkButton logClearButton;
        private DarkUI.Controls.DarkButton logSaveButton;
        public DarkUI.Controls.DarkTextBox logTextBox;
        public DarkUI.Controls.DarkCheckBox logTimestampCheckBox;
        public DarkUI.Controls.DarkCheckBox alwaysOnTopCheckBox;
    }
}