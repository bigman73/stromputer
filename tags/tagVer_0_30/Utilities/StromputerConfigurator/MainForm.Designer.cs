namespace StromputerConfig
{
    partial class MainForm
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
            this.sendIsAliveButton = new System.Windows.Forms.Button();
            this.initSerialButton = new System.Windows.Forms.Button();
            this.responseTextBox = new System.Windows.Forms.TextBox();
            this.sendTempButton = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // sendIsAliveButton
            // 
            this.sendIsAliveButton.Location = new System.Drawing.Point(35, 50);
            this.sendIsAliveButton.Name = "sendIsAliveButton";
            this.sendIsAliveButton.Size = new System.Drawing.Size(75, 23);
            this.sendIsAliveButton.TabIndex = 0;
            this.sendIsAliveButton.Text = "Send IsAlive";
            this.sendIsAliveButton.UseVisualStyleBackColor = true;
            this.sendIsAliveButton.Click += new System.EventHandler(this.sendIsAliveButton_Click);
            // 
            // initSerialButton
            // 
            this.initSerialButton.Location = new System.Drawing.Point(35, 21);
            this.initSerialButton.Name = "initSerialButton";
            this.initSerialButton.Size = new System.Drawing.Size(75, 23);
            this.initSerialButton.TabIndex = 1;
            this.initSerialButton.Text = "Initialize Serial";
            this.initSerialButton.UseVisualStyleBackColor = true;
            this.initSerialButton.Click += new System.EventHandler(this.initSerialButton_Click);
            // 
            // responseTextBox
            // 
            this.responseTextBox.Location = new System.Drawing.Point(132, 21);
            this.responseTextBox.Multiline = true;
            this.responseTextBox.Name = "responseTextBox";
            this.responseTextBox.Size = new System.Drawing.Size(410, 354);
            this.responseTextBox.TabIndex = 2;
            // 
            // sendTempButton
            // 
            this.sendTempButton.Location = new System.Drawing.Point(35, 79);
            this.sendTempButton.Name = "sendTempButton";
            this.sendTempButton.Size = new System.Drawing.Size(75, 23);
            this.sendTempButton.TabIndex = 3;
            this.sendTempButton.Text = "Send Temp";
            this.sendTempButton.UseVisualStyleBackColor = true;
            this.sendTempButton.Click += new System.EventHandler(this.sendTempButton_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(563, 407);
            this.Controls.Add(this.sendTempButton);
            this.Controls.Add(this.responseTextBox);
            this.Controls.Add(this.initSerialButton);
            this.Controls.Add(this.sendIsAliveButton);
            this.Name = "Form1";
            this.Text = "Stromputer Configurator";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button sendIsAliveButton;
        private System.Windows.Forms.Button initSerialButton;
        private System.Windows.Forms.TextBox responseTextBox;
        private System.Windows.Forms.Button sendTempButton;
    }
}

