import { Component, OnInit } from '@angular/core';
import { ViewChild, ElementRef, AfterViewInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { SolverService } from '../../services/solver';
import { BackendResponse } from '../../models/Response';

@Component({
  selector: 'app-tsp-map',
  templateUrl: './tsp-map.html',
  styleUrl: './tsp-map.scss',
  imports: [CommonModule, FormsModule]
})
export class TspMapComponent implements AfterViewInit, OnInit {
  @ViewChild('mapCanvas') canvas!: ElementRef<HTMLCanvasElement>;
  private ctx!: CanvasRenderingContext2D;
  public cities: { x: number; y: number }[] = [];
  public distances: number[][] = [];
  public alpha: number = 1;
  public beta: number = 2;
  public rho: number = 0.1;
  public showTable: boolean = true;

  private readonly CIRCLE_RADIUS = 10;
  private readonly CLICK_TOLERANCE = 15;
  private readonly FONT = '12px Arial';
  private readonly ROUNDING_PRECISION = 100;
  private readonly PATH_WIDTH = 8;

  private offsetX: number = 0;
  private offsetY: number = 0;
  private scale: number = 1;
  private isDragging: boolean = false;
  private hasDragged: boolean = false;
  private lastMouseX: number = 0;
  private lastMouseY: number = 0;
  private dragThreshold: number = 5;
  private currentPath: number[] | null = null;

  private undoStack: { x: number; y: number }[][] = [];
  private redoStack: { x: number; y: number }[][] = [];

  constructor(private solverService: SolverService) {}

  ngOnInit(): void {
    this.solverService.getSolverUpdates().subscribe(msg => {
      console.log('Received from backend:', msg);
      let response: BackendResponse = msg as BackendResponse;
      let path: number[] = response.payload;
      this.currentPath = path;
      this.redraw();
    });
  }

  ngAfterViewInit(): void {
    const canvasEl = this.canvas.nativeElement;
    
    canvasEl.width = canvasEl.offsetWidth;
    canvasEl.height = canvasEl.offsetHeight;
    this.ctx = canvasEl.getContext('2d')!;

    canvasEl.addEventListener('mousemove', (event) => {
      const { x, y } = this.getMousePos(event);

      if (this.isDragging) {
        const deltaX = x - this.lastMouseX;
        const deltaY = y - this.lastMouseY;
        if (Math.abs(deltaX) > this.dragThreshold || Math.abs(deltaY) > this.dragThreshold) {
          this.hasDragged = true;
          canvasEl.style.cursor = 'grabbing';
          this.offsetX += deltaX;
          this.offsetY += deltaY;
          this.lastMouseX = x;
          this.lastMouseY = y;
          this.redraw();
        }
      } else {
        const worldX = (x - this.offsetX) / this.scale;
        const worldY = (y - this.offsetY) / this.scale;
        const isNearCircle = this.cities.some(circle => {
          const distance = Math.sqrt((circle.x - worldX) ** 2 + (circle.y - worldY) ** 2);
          return distance < this.CLICK_TOLERANCE / this.scale;
        });
        canvasEl.style.cursor = isNearCircle ? 'pointer' : 'grab';
      }
    });

    canvasEl.addEventListener('mousedown', (event) => {
      const { x, y } = this.getMousePos(event);
      const worldX = (x - this.offsetX) / this.scale;
      const worldY = (y - this.offsetY) / this.scale;

      const index = this.cities.findIndex(circle => {
        const distance = Math.sqrt((circle.x - worldX) ** 2 + (circle.y - worldY) ** 2);
        return distance < this.CLICK_TOLERANCE / this.scale;
      });

      if (index !== -1) {
        this.saveState();
        this.cities.splice(index, 1);
        this.currentPath = null;
        this.recalculateDistances();
        this.redraw();
      } else {
        this.isDragging = true;
        this.hasDragged = false;
        this.lastMouseX = x;
        this.lastMouseY = y;
      }
    });

    canvasEl.addEventListener('mouseup', (event) => {
      if (this.isDragging) {
        if (!this.hasDragged) {
          const { x, y } = this.getMousePos(event);
          const worldX = (x - this.offsetX) / this.scale;
          const worldY = (y - this.offsetY) / this.scale;
          if (isFinite(worldX) && isFinite(worldY)) {
            this.saveState();
            this.cities.push({ x: worldX, y: worldY });
            this.currentPath = null;
            this.redraw();
            this.recalculateDistances();
          }
        }
        this.isDragging = false;
        this.hasDragged = false;
        canvasEl.style.cursor = 'grab';
      }
    });

    canvasEl.addEventListener('wheel', (event) => {
      event.preventDefault();
      const { x, y } = this.getMousePos(event);
      const zoomFactor = Math.pow(1.5, -event.deltaY / 100);
      this.zoomAt(x, y, zoomFactor);
    });

    canvasEl.addEventListener('contextmenu', (event) => event.preventDefault());

    document.addEventListener('keydown', (event) => {
      if (event.ctrlKey || event.metaKey) {
        if (event.key === 'z' && !event.shiftKey) {
          event.preventDefault();
          this.undo();
        } else if ((event.key === 'y') || (event.key === 'z' && event.shiftKey)) {
          event.preventDefault();
          this.redo();
        }
      }
    });

    this.redraw();
  }

  private getMousePos(event: MouseEvent): { x: number; y: number } {
    const canvasEl = this.canvas.nativeElement;
    const rect = canvasEl.getBoundingClientRect();
    if (rect.width === 0 || rect.height === 0) return { x: 0, y: 0 };
    const scaleX = canvasEl.width / rect.width;
    const scaleY = canvasEl.height / rect.height;
    return {
      x: (event.clientX - rect.left) * scaleX,
      y: (event.clientY - rect.top) * scaleY
    };
  }

  private zoomAt(canvasX: number, canvasY: number, factor: number) {
    const worldX = (canvasX - this.offsetX) / this.scale;
    const worldY = (canvasY - this.offsetY) / this.scale;
    this.scale *= factor;
    this.scale = Math.max(0.1, Math.min(2, this.scale));
    this.offsetX = canvasX - worldX * this.scale;
    this.offsetY = canvasY - worldY * this.scale;
    this.redraw();
  }

  zoomIn() {
    const centerX = this.canvas.nativeElement.width / 2;
    const centerY = this.canvas.nativeElement.height / 2;
    this.zoomAt(centerX, centerY, 1.2);
  }

  zoomOut() {
    const centerX = this.canvas.nativeElement.width / 2;
    const centerY = this.canvas.nativeElement.height / 2;
    this.zoomAt(centerX, centerY, 1 / 1.2);
  }

  private saveState() {
    this.undoStack.push([...this.cities]);
    this.redoStack = [];
  }

  undo() {
    if (this.undoStack.length > 0) {
      this.redoStack.push([...this.cities]);
      this.cities = this.undoStack.pop()!;
      this.currentPath = null;
      this.recalculateDistances();
      this.redraw();
    }
  }

  redo() {
    if (this.redoStack.length > 0) {
      this.undoStack.push([...this.cities]);
      this.cities = this.redoStack.pop()!;
      this.currentPath = null;
      this.recalculateDistances();
      this.redraw();
    }
  }

  private drawCircle(x: number, y: number, index: number) {
    this.ctx.beginPath();
    this.ctx.arc(x, y, this.CIRCLE_RADIUS * this.scale, 0, 2 * Math.PI);
    this.ctx.fillStyle = 'red';
    this.ctx.fill();

    this.ctx.fillStyle = 'white';
    this.ctx.font = `${12 * this.scale}px Arial`;
    this.ctx.textAlign = 'center';
    this.ctx.textBaseline = 'middle';
    this.ctx.fillText((index + 1).toString(), x, y);
  }

  private drawFixedCircle(x: number, y: number, index: number) {
    this.ctx.beginPath();
    this.ctx.arc(x, y, this.CIRCLE_RADIUS, 0, 2 * Math.PI);
    this.ctx.fillStyle = 'red';
    this.ctx.fill();

    this.ctx.fillStyle = 'white';
    this.ctx.font = '12px Arial';
    this.ctx.textAlign = 'center';
    this.ctx.textBaseline = 'middle';
    this.ctx.fillText((index + 1).toString(), x, y);
  }

  private drawCalculatedPath(pathNodeIndices: number[]){
    this.currentPath = pathNodeIndices;
    this.redraw();
  }

  private drawGrid() {
    this.ctx.strokeStyle = 'lightgray';
    this.ctx.lineWidth = 1 / this.scale;
    const gridSize = 100;

    const startX = Math.floor((-this.offsetX / this.scale) / gridSize) * gridSize;
    const endX = Math.ceil(((this.canvas.nativeElement.width - this.offsetX) / this.scale) / gridSize) * gridSize;
    for (let x = startX; x <= endX; x += gridSize) {
      this.ctx.beginPath();
      this.ctx.moveTo(x, -10000);
      this.ctx.lineTo(x, 10000);
      this.ctx.stroke();
    }

    const startY = Math.floor((-this.offsetY / this.scale) / gridSize) * gridSize;
    const endY = Math.ceil(((this.canvas.nativeElement.height - this.offsetY) / this.scale) / gridSize) * gridSize;
    for (let y = startY; y <= endY; y += gridSize) {
      this.ctx.beginPath();
      this.ctx.moveTo(-10000, y);
      this.ctx.lineTo(10000, y);
      this.ctx.stroke();
    }
  }

  private redraw() {
    this.ctx.clearRect(0, 0, this.canvas.nativeElement.width, this.canvas.nativeElement.height);
    this.ctx.save();
    this.ctx.translate(this.offsetX, this.offsetY);
    this.ctx.scale(this.scale, this.scale);

    this.drawGrid();

    if (this.currentPath && this.cities.length >= 2) {
      this.ctx.strokeStyle = 'black';
      this.ctx.lineWidth = 3 / this.scale;
      for (let i = 0; i < this.currentPath.length - 1; i++) {
        const current = this.cities[this.currentPath[i]];
        const next = this.cities[this.currentPath[i + 1]];
        this.ctx.beginPath();
        this.ctx.moveTo(current.x, current.y);
        this.ctx.lineTo(next.x, next.y);
        this.ctx.stroke();
      }
    }
    this.ctx.restore();
    
    this.cities.forEach((circle, index) => {
      const screenX = this.offsetX + circle.x * this.scale;
      const screenY = this.offsetY + circle.y * this.scale;
      this.drawFixedCircle(screenX, screenY, index);
    });
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
          let dx = to.x - from.x;
          let dy = to.y - from.y;
          let dist = Math.sqrt(dx * dx + dy * dy);
          if (!isFinite(dist)) dist = 0;
          this.distances[i][j] = Math.round(dist * this.ROUNDING_PRECISION) / this.ROUNDING_PRECISION;
        }
        j++;
      });
      i++;
    });
  }

  sendDistances() {
    if (this.distances.length > 0) {
      this.solverService.sendTSPData(this.distances, this.alpha, this.beta, this.rho);
    }
  }
}