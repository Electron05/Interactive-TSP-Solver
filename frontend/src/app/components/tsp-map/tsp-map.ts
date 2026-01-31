import { Component, OnInit } from '@angular/core';
import { ViewChild, ElementRef, AfterViewInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { SolverService } from '../../services/solver';
import { BackendResponse } from '../../models/Response';

@Component({
  selector: 'app-tsp-map',
  templateUrl: './tsp-map.html',
  styleUrl: './tsp-map.scss',
  imports: [CommonModule]
})
export class TspMapComponent implements AfterViewInit, OnInit {
  @ViewChild('mapCanvas') canvas!: ElementRef<HTMLCanvasElement>;
  private ctx!: CanvasRenderingContext2D;
  public cities: { x: number; y: number }[] = []; // Store cities positions
  public distances: number[][] = [];

  private readonly CIRCLE_RADIUS = 10;
  private readonly CLICK_TOLERANCE = 15;
  private readonly FONT = '12px Arial';
  private readonly ROUNDING_PRECISION = 100;
    private readonly PATH_WIDTH = 12;

  constructor(private solverService: SolverService) {}

  ngOnInit(): void {
    this.solverService.getSolverUpdates().subscribe(msg => {
      console.log('Received from backend:', msg);
      let response: BackendResponse = msg as BackendResponse;
      let path: number[] = JSON.parse(response.payload);
      this.drawCalculatedPath(path);
    });
  }

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

      const isNearCircle = this.cities.some(circle => {
        const distance = Math.sqrt((circle.x - x) ** 2 + (circle.y - y) ** 2);
        return distance < this.CLICK_TOLERANCE;
      });

      canvasEl.style.cursor = isNearCircle ? 'pointer' : 'crosshair';
    });

    canvasEl.addEventListener('click', (event) => {
      const rect = canvasEl.getBoundingClientRect();
      const scaleX = canvasEl.width / rect.width;
      const scaleY = canvasEl.height / rect.height;
      const x = (event.clientX - rect.left) * scaleX;
      const y = (event.clientY - rect.top) * scaleY;

      const index = this.cities.findIndex(circle => {
        const distance = Math.sqrt((circle.x - x) ** 2 + (circle.y - y) ** 2);
        return distance < this.CLICK_TOLERANCE; // Tolerance
      });

      if (index !== -1) {
        this.cities.splice(index, 1);
      } else {
        this.cities.push({ x, y });
        canvasEl.style.cursor = 'pointer';
      }

      this.redraw();
      this.recalculateDistances();
    });
  }

  private drawCircle(x: number, y: number, index: number) {
    this.ctx.beginPath();
    this.ctx.arc(x, y, this.CIRCLE_RADIUS, 0, 2 * Math.PI);
    this.ctx.fillStyle = 'red';
    this.ctx.fill();

    this.ctx.fillStyle = 'white';
    this.ctx.font = this.FONT;
    this.ctx.textAlign = 'center';
    this.ctx.textBaseline = 'middle';
    this.ctx.fillText((index + 1).toString(), x, y);
  }

  private drawCalculatedPath(pathNodeIndices: number[]){
    if(this.cities.length < 2) return;
    this.ctx.beginPath();
    this.ctx.lineWidth = this.PATH_WIDTH;
    this.ctx.moveTo(this.cities[0].x, this.cities[0].y);
    for(let i = 1; i < pathNodeIndices.length; i++){
      this.ctx.lineTo(this.cities[i].x, this.cities[i].y);
    }
    this.ctx.stroke();
  }

  private redraw() {
    this.ctx.clearRect(0, 0, this.canvas.nativeElement.width, this.canvas.nativeElement.height);
    this.cities.forEach((circle,index) => this.drawCircle(circle.x, circle.y,index));
    
  }

  private recalculateDistances(){
    this.distances = [];

    let i = 0;
    this.cities.forEach(from => {
      this.distances.push(new Array(this.cities.length));
      let j = 0;
      this.cities.forEach(to => {
        if(i==j){
          this.distances[i][j] = 0;
        }
        else{
          this.distances[i][j] = Math.round(
            Math.sqrt(
              (to.x - from.x)**2 +
              (to.y - from.y)**2
            ) * this.ROUNDING_PRECISION
          ) / this.ROUNDING_PRECISION;
        }
        j++;
      });
      i++;
    });
  }

  sendDistances() {
    if (this.distances.length > 0) {
      this.solverService.sendTSPData(this.distances);
    }
  }
}