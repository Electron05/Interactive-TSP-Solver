import { Component } from '@angular/core';
import { ViewChild, ElementRef, AfterViewInit } from '@angular/core';
import { CommonModule } from '@angular/common';

@Component({
  selector: 'app-tsp-map',
  templateUrl: './tsp-map.html',
  styleUrl: './tsp-map.scss',
  imports: [CommonModule]
})
export class TspMapComponent implements AfterViewInit {
  @ViewChild('mapCanvas') canvas!: ElementRef<HTMLCanvasElement>;
  private ctx!: CanvasRenderingContext2D;
  public circles: { x: number; y: number }[] = []; // Store circle positions
  public distances: number[][] = [];

  ngAfterViewInit(): void {
    const canvasEl = this.canvas.nativeElement;
    
    canvasEl.width = canvasEl.offsetWidth;
    canvasEl.height = canvasEl.offsetHeight;
    this.ctx = canvasEl.getContext('2d')!;

    canvasEl.addEventListener('mousemove', (event) => {
      const rect = canvasEl.getBoundingClientRect();
      const scaleX = canvasEl.width / rect.width;
      const scaleY = canvasEl.height / rect.height;
      const x = (event.clientX - rect.left) * scaleX;
      const y = (event.clientY - rect.top) * scaleY;

      const isNearCircle = this.circles.some(circle => {
        const distance = Math.sqrt((circle.x - x) ** 2 + (circle.y - y) ** 2);
        return distance < 15;
      });

      canvasEl.style.cursor = isNearCircle ? 'pointer' : 'crosshair';
    });

    canvasEl.addEventListener('click', (event) => {
      const rect = canvasEl.getBoundingClientRect();
      const scaleX = canvasEl.width / rect.width;
      const scaleY = canvasEl.height / rect.height;
      const x = (event.clientX - rect.left) * scaleX;
      const y = (event.clientY - rect.top) * scaleY;

      const index = this.circles.findIndex(circle => {
        const distance = Math.sqrt((circle.x - x) ** 2 + (circle.y - y) ** 2);
        return distance < 15; // Tolerance
      });

      if (index !== -1) {
        this.circles.splice(index, 1);
      } else {
        this.circles.push({ x, y });
        canvasEl.style.cursor = 'pointer';
      }

      this.redraw();
      this.recalculateDistances();
    });
  }

  private drawCircle(x: number, y: number, index: number) {
    this.ctx.beginPath();
    this.ctx.arc(x, y, 10, 0, 2 * Math.PI);
    this.ctx.fillStyle = 'red';
    this.ctx.fill();

    this.ctx.fillStyle = 'white';
    this.ctx.font = '12px Arial';
    this.ctx.textAlign = 'center';
    this.ctx.textBaseline = 'middle';
    this.ctx.fillText((index + 1).toString(), x, y);
  }

  private redraw() {
    this.ctx.clearRect(0, 0, this.canvas.nativeElement.width, this.canvas.nativeElement.height);
    this.circles.forEach((circle,index) => this.drawCircle(circle.x, circle.y,index));
    
  }

  private recalculateDistances(){
    this.distances = [];

    var i = 0;
    this.circles.forEach(from => {
      this.distances.push(new Array(this.circles.length));
      var j = 0;
      this.circles.forEach(to => {
        if(i==j){
          this.distances[i][j] = 0;
        }
        else{
          this.distances[i][j] = Math.round(
            Math.sqrt(
              Math.pow(to.x - from.x, 2) +
              Math.pow(to.y - from.y, 2)
            ) * 100
          ) / 100;
        }
        j++;
      });
      i++;
    });
  }
}